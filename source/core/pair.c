#include "pair.h"
#include "snd.h"
#include "bt.h"
#include "dbs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define PAIR_UPDATE_POLL_US 100000
#define DATA_UPDATE_POLL_US 100000

#define TRACK_TITLE_LENGTH_MAX 128

#define THREAD_STATUS_RUNNING 0
#define THREAD_STATUS_CLOSING 1
#define THREAD_STATUS_END 2

//data
static char pair_sndInputDevice[128];
static char pair_deviceMac[18];
static char pair_trackTitle[TRACK_TITLE_LENGTH_MAX];
static char pair_trackAlbum[TRACK_TITLE_LENGTH_MAX];
static char pair_trackArtist[TRACK_TITLE_LENGTH_MAX];
static int pair_trackDuration;
static int pair_trackPosition;
static int pair_mediaState;

//devices thread
static char pair_processBluetoothDevicesThreadStatus = THREAD_STATUS_END;
static void* pair_processBluetoothDevices(void* args);
static void pair_startBluetoothDevicesThread();
static void pair_stopBluetoothDevicesThread();

//media state thread
static char pair_mediaStateThreadStatus = THREAD_STATUS_END;
static void* pair_updateMediaState(void* args);
static void pair_startMediaStateThread();
static void pair_stopMediaStateThread();

//helper functions
static void pair_setDevice(bt_device* device);
static void pair_unsetDevice(bt_device* device);
static void pair_runMediaCommand(const char* command);
static void pair_readFromBuffer(char* buffer, const char* find, const char* after, char* read, int max);
static void pair_clearMediaTrackData();
static void pair_usleep(long useconds);

// Setup and initialize the BT Pairing utils
int pair_init()
{
	//check dependencies
	if(!bt_isInit() || !snd_isInit()) {
		if(!bt_isInit()) fprintf(stderr, "pair_init: Bluetooth Utils dependency is not initialized\n");
		if(!snd_isInit()) fprintf(stderr, "pair_init: Sound Utils dependency is not initialized\n");
		if(!dbs_isInit()) fprintf(stderr, "pair_init: DBus Utils dependency is not initialized\n");
		pair_close();
		return 1;
	}
	
	//data
	pair_deviceMac[0] = 0;
	pair_mediaState = PAIR_MEDIA_STATE_NOT_CONNECTED;
	pair_clearMediaTrackData();
	
	pair_startBluetoothDevicesThread();
	pair_startMediaStateThread();
	return 0;
}

// Checks if the BT Pairing utils are initialized
char pair_isInit()
{
	if(pair_processBluetoothDevicesThreadStatus == THREAD_STATUS_RUNNING && pair_mediaStateThreadStatus == THREAD_STATUS_RUNNING) return 1;
	return 0;
}

// Resets to the pairing state
void pair_disconnectDevice()
{
	pair_stopBluetoothDevicesThread();	
	pair_startBluetoothDevicesThread();
}

// Sends the play command to the connected media device
void pair_mediaPlay()
{
	pair_runMediaCommand("Play");
}

// Sends the pause command to the connected media device
void pair_mediaPause()
{
	pair_runMediaCommand("Pause");
}

// Sends the next command to the connected media device
void pair_mediaNext()
{
	pair_runMediaCommand("Next");
}

// Sends the previous command to the connected media device
void pair_mediaPrevious()
{
	pair_runMediaCommand("Previous");
}

// Gets the state of the connected media device
int pair_mediaGetState()
{
	return pair_mediaState;
}

// Gets the track position of the connected media device (ms)
int pair_mediaGetTrackPosition()
{
	return pair_trackPosition;
}

// Gets the track duration of the connected media device (ms)
int pair_mediaGetTrackDuration()
{
	return pair_trackPosition;
}

// Gets the track title of the connected media device
const char* pair_mediaGetTrackTitle()
{
	return pair_trackTitle;
}

// Gets the track album of the connected media device
const char* pair_mediaGetTrackAlbum()
{
	return pair_trackAlbum;
}

// Gets the track artist of the connected media device
const char* pair_mediaGetTrackArtist()
{
	return pair_trackArtist;
}

// Cleans up the BT Pairing utils
int pair_close()
{
	pair_stopBluetoothDevicesThread();
	pair_stopMediaStateThread();
	return 0;
}

// Bluetooth Devices Proccessing Thread
static void* pair_processBluetoothDevices(void* args)
{
	int i;
	bt_device selectedDevice;
	selectedDevice.mac[0] = 0;
	bt_discoverableOn();
	
	while(1==1) {
		//exit?
		if(pair_processBluetoothDevicesThreadStatus) {
			pair_unsetDevice(&selectedDevice);
			break;
		}
		if(selectedDevice.mac[0]) {
			
			//make sure device is still connected
			bt_getDevicesDetails(&selectedDevice);
			if(!selectedDevice.paired || !selectedDevice.trusted || !selectedDevice.connected) {
				
				//unset device
				pair_unsetDevice(&selectedDevice);
				snd_playFile("data/device-removed.wav");
				bt_discoverableOn();
			}
		} else {
		
			//loop through devices to try to connect new audio device
			bt_device** deviceList = bt_getDevices();
			for(i=0; deviceList[i]; i++) {
				
				//look for new device to trust and connect
				if(deviceList[i]->paired && deviceList[i]->type==BT_DEVICE_TYPE_AUDIOSOURCE) {
					if(!deviceList[i]->trusted) bt_trustDevice(deviceList[i]->mac);
					if(!deviceList[i]->connected) bt_connectDevice(deviceList[i]->mac);
				}
				
				//look for a connected device to select
				if(deviceList[i]->paired && deviceList[i]->trusted && deviceList[i]->connected && deviceList[i]->type==BT_DEVICE_TYPE_AUDIOSOURCE) {
					
					//set device
					bt_discoverableOff();
					snd_playFile("data/device-added.wav");
					strcpy(selectedDevice.mac, deviceList[i]->mac);
					pair_setDevice(&selectedDevice);
					break;
				}
			}
		}
			
		pair_usleep(PAIR_UPDATE_POLL_US);
	}
	
	pair_processBluetoothDevicesThreadStatus = THREAD_STATUS_END;
	return 0;
}
static void pair_startBluetoothDevicesThread()
{
	pthread_t threadId;
	pair_stopBluetoothDevicesThread();
	
	pair_processBluetoothDevicesThreadStatus = THREAD_STATUS_RUNNING;
    pthread_create(&threadId, NULL, pair_processBluetoothDevices, NULL);
}
static void pair_stopBluetoothDevicesThread()
{
	if(pair_processBluetoothDevicesThreadStatus == THREAD_STATUS_RUNNING) pair_processBluetoothDevicesThreadStatus = THREAD_STATUS_CLOSING;
	while(pair_processBluetoothDevicesThreadStatus != THREAD_STATUS_END) pair_usleep(10000);
}

// Media State Update Thread
static void* pair_updateMediaState(void* args)
{
	int i;
	char buffer[1024];
	char* pos;
	
	while(1==1) {
		//exit?
		if(pair_mediaStateThreadStatus) {
			pair_clearMediaTrackData();
			if(pair_deviceMac[0] != 0) pair_mediaState = PAIR_MEDIA_STATE_UNKNOWN;
			else pair_mediaState = PAIR_MEDIA_STATE_NOT_CONNECTED;
			break;
		}
		
		//no device?
		if(pair_deviceMac[0] == 0) {
			pair_clearMediaTrackData();
			pair_mediaState = PAIR_MEDIA_STATE_NOT_CONNECTED;
			
		} else {
			
			//determine object path
			char objectPath[50];
			strcpy(objectPath, "/org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx/player0");
			for(i=0; i<6; i++) {
				objectPath[20+(i*3)] = pair_deviceMac[(i*3)+0];
				objectPath[21+(i*3)] = pair_deviceMac[(i*3)+1];
			}
			
			//get status
			dbs_complexMethodCall(buffer, 1024, objectPath, "org.bluez", "org.freedesktop.DBus.Properties", "Get", "org.bluez.MediaPlayer1", "Status", NULL);
			if(strstr(buffer, "string:playing") != NULL) pair_mediaState = PAIR_MEDIA_STATE_PLAYING;
			else if(strstr(buffer, "string:stopped") != NULL) pair_mediaState = PAIR_MEDIA_STATE_STOPPED;
			else if(strstr(buffer, "string:paused") != NULL) pair_mediaState = PAIR_MEDIA_STATE_PAUSED;
			else if(strstr(buffer, "string:forward-seek") != NULL) pair_mediaState = PAIR_MEDIA_STATE_FORWARD_SEEK;
			else if(strstr(buffer, "string:reverse-seek") != NULL) pair_mediaState = PAIR_MEDIA_STATE_REVERSE_SEEK;
			else if(strstr(buffer, "string:error") != NULL) pair_mediaState = PAIR_MEDIA_STATE_ERROR;
			
			//get position
			dbs_complexMethodCall(buffer, 1024, objectPath, "org.bluez", "org.freedesktop.DBus.Properties", "Get", "org.bluez.MediaPlayer1", "Position", NULL);
			if((pos=strstr(buffer, "uint32:")) != NULL) {
				pair_trackPosition = atoi(pos + strlen("uint32:"));
			}
			
			//get track info
			char temp[16];
			dbs_complexMethodCall(buffer, 1024, objectPath, "org.bluez", "org.freedesktop.DBus.Properties", "Get", "org.bluez.MediaPlayer1", "Track", NULL);
			pair_readFromBuffer(buffer, "string:Title", "string:", pair_trackTitle, TRACK_TITLE_LENGTH_MAX);
			pair_readFromBuffer(buffer, "string:Album", "string:", pair_trackAlbum, TRACK_TITLE_LENGTH_MAX);
			pair_readFromBuffer(buffer, "string:Artist", "string:", pair_trackArtist, TRACK_TITLE_LENGTH_MAX);
			pair_readFromBuffer(buffer, "string:Duration", "uint32:", temp, 16);
			if(temp[0] != 0) pair_trackDuration = atoi(temp);
			else pair_trackDuration = -1;
		}
		
		pair_usleep(DATA_UPDATE_POLL_US);
	}
	
	pair_mediaStateThreadStatus = THREAD_STATUS_END;
	return 0;
}
static void pair_startMediaStateThread()
{
	pthread_t threadId;
	pair_stopMediaStateThread();
	
	pair_mediaStateThreadStatus = THREAD_STATUS_RUNNING;
    pthread_create(&threadId, NULL, pair_updateMediaState, NULL);
}
static void pair_stopMediaStateThread()
{
	if(pair_mediaStateThreadStatus == THREAD_STATUS_RUNNING) pair_mediaStateThreadStatus = THREAD_STATUS_CLOSING;
	while(pair_mediaStateThreadStatus != THREAD_STATUS_END) pair_usleep(10000);
}

//helper functions
static void pair_setDevice(bt_device* device) {
	pair_deviceMac[0] = 0;
	if(device && device->mac[0]) {
		strcpy(pair_deviceMac, device->mac);
		sprintf(pair_sndInputDevice, "bluealsa:DEV=%s", device->mac);
		snd_setInputDevice(pair_sndInputDevice);
	}
}
static void pair_unsetDevice(bt_device* device) {
	pair_deviceMac[0] = 0;
	if(device && device->mac[0]) {
		snd_setInputDevice(0);
		bt_removeDevice(device->mac);
		device->mac[0] = 0;
	}
}
static void pair_runMediaCommand(const char* command) {
	if(pair_deviceMac[0] != 0) {
		int i;
		char objectPath[50];
		strcpy(objectPath, "/org/bluez/hci0/dev_xx_xx_xx_xx_xx_xx/player0");
		for(i=0; i<6; i++) {
			objectPath[20+(i*3)] = pair_deviceMac[(i*3)+0];
			objectPath[21+(i*3)] = pair_deviceMac[(i*3)+1];
		}
		dbs_simpleMethodCall(objectPath, "org.bluez", "org.bluez.MediaPlayer1", command);
	}
}
static void pair_readFromBuffer(char* buffer, const char* find, const char* after, char* read, int max) {
	char* pos = buffer;
	int i = 0;
	
	read[0] = 0;
	if((pos=strstr(buffer, find)) != NULL) {
		pos += strlen(find);
		if((pos=strstr(pos, after)) != NULL) {
			pos += strlen(after);
			int end = strlen(pos);
			for(i=0; pos[i]!='\n' && i<end && i<(max-1); i++) read[i] = pos[i];
		}
	}
	read[i] = 0;
}
static void pair_clearMediaTrackData() {
	pair_trackTitle[0] = 0;
	pair_trackAlbum[0] = 0;
	pair_trackArtist[0] = 0;
	pair_trackDuration = -1;
	pair_trackPosition = -1;
}
static void pair_usleep(long useconds) {
	long seconds = useconds / 1000000;
	useconds = useconds - (seconds*1000000);
	
	struct timespec ts;
	ts.tv_sec = seconds;
	ts.tv_nsec = useconds * 1000;
	nanosleep(&ts, &ts);
}
