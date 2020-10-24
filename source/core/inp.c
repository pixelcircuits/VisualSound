#include "inp.h"
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <linux/input.h>

#define MAX_DEVICES 16
#define STATE_UPDATE_MILLIS 8
#define DEVICE_UPDATE_MILLIS 5000

#define KEY_DEFAULT_CODE_UP        103 /* Up Arrow */
#define KEY_DEFAULT_CODE_DOWN      108 /* Down Arrow */
#define KEY_DEFAULT_CODE_LEFT      105 /* Left Arrow */
#define KEY_DEFAULT_CODE_RIGHT     106 /* Right Arrow */
#define KEY_DEFAULT_CODE_HOME      57  /* Space Key */
#define KEY_DEFAULT_CODE_MENU      50  /* M Key */
#define KEY_DEFAULT_CODE_BACK      14  /* Backspace Key */
#define KEY_DEFAULT_CODE_OK        28  /* Enter Key */
#define KEY_DEFAULT_CODE_VOLUP     13  /* Plus Key */
#define KEY_DEFAULT_CODE_VOLDOWN   12  /* Minus Key */
#define KEY_DEFAULT_CODE_PWR       1   /* Esc Key */

// Data
static char inp_isInitFlag = 0;
static char inp_deviceSize = 0;
static int inp_deviceFd[MAX_DEVICES];
static char* inp_deviceName[MAX_DEVICES];
static clock_t inp_lastDeviceUpdate = 0;
static int inp_keyPresses[INP_BTN_TOTAL];
static int inp_keyCodes[INP_BTN_TOTAL];
static char inp_keyHold[INP_BTN_TOTAL];
static char inp_keyTouched[INP_BTN_TOTAL];
static unsigned char inp_buttonStates[INP_BTN_TOTAL];
static pthread_t inp_threadId = -1;

// Helper Functions
static void inp_checkButtonInput();
static int inp_addDeviceToList(char* path);
static int inp_getDeviceFromList(char* path);
static void inp_removeDeviceFromList(int index);
static void inp_updateDeviceList();
static void inp_clearDeviceList();

// State polling thread
void* inp_thread_statePolling(void* args)
{
	while(inp_isInitFlag > 0) {
		inp_checkButtonInput();
		
		//sleep
		struct timespec ts;
		ts.tv_sec = STATE_UPDATE_MILLIS / 1000;
		ts.tv_nsec = (STATE_UPDATE_MILLIS % 1000) * 1000000;
		nanosleep(&ts, &ts);
	}
	return 0;
}

// Setup and initialize the Controller interface
int inp_init()
{
	//already initialized?
	if(inp_isInitFlag == 1) return 0;
	
	//data
	int i;
	for(i=0; i<INP_BTN_TOTAL; i++) inp_keyPresses[i] = 0;
	for(i=0; i<INP_BTN_TOTAL; i++) inp_buttonStates[i] = 0;
	inp_keyCodes[INP_BTN_UP] = KEY_DEFAULT_CODE_UP;
	inp_keyCodes[INP_BTN_DOWN] = KEY_DEFAULT_CODE_DOWN;
	inp_keyCodes[INP_BTN_LEFT] = KEY_DEFAULT_CODE_LEFT;
	inp_keyCodes[INP_BTN_RIGHT] = KEY_DEFAULT_CODE_RIGHT;
	inp_keyCodes[INP_BTN_HOME] = KEY_DEFAULT_CODE_HOME;
	inp_keyCodes[INP_BTN_MENU] = KEY_DEFAULT_CODE_MENU;
	inp_keyCodes[INP_BTN_BACK] = KEY_DEFAULT_CODE_BACK;
	inp_keyCodes[INP_BTN_OK] = KEY_DEFAULT_CODE_OK;
	inp_keyCodes[INP_BTN_VOLUP] = KEY_DEFAULT_CODE_VOLUP;
	inp_keyCodes[INP_BTN_VOLDOWN] = KEY_DEFAULT_CODE_VOLDOWN;
	inp_keyCodes[INP_BTN_PWR] = KEY_DEFAULT_CODE_PWR;	
	
	//initial device search
	inp_updateDeviceList();
	
	//start thread
	inp_isInitFlag = 1;
    if(pthread_create(&inp_threadId, NULL, inp_thread_statePolling, NULL) > 0) {
		inp_close();
        return 1;
    }
	
	return 0;
}

// Checks if the Controller interface is initialized
char inp_isInit()
{
	return inp_isInitFlag;
}

// Updates the button holding state
void inp_updateButtonState()
{
	if(inp_keyPresses[INP_BTN_UP] > 0 || (inp_keyHold[INP_BTN_UP] && inp_keyTouched[INP_BTN_UP])) {
		if(inp_buttonStates[INP_BTN_UP] < 255) inp_buttonStates[INP_BTN_UP]++;
	} else inp_buttonStates[INP_BTN_UP] = 0;
	
	if(inp_keyPresses[INP_BTN_DOWN] > 0 || (inp_keyHold[INP_BTN_DOWN] && inp_keyTouched[INP_BTN_DOWN])) {
		if(inp_buttonStates[INP_BTN_DOWN] < 255) inp_buttonStates[INP_BTN_DOWN]++;
	} else inp_buttonStates[INP_BTN_DOWN] = 0;
	
	if(inp_keyPresses[INP_BTN_LEFT] > 0 || (inp_keyHold[INP_BTN_LEFT] && inp_keyTouched[INP_BTN_LEFT])) {
		if(inp_buttonStates[INP_BTN_LEFT] < 255) inp_buttonStates[INP_BTN_LEFT]++;
	} else inp_buttonStates[INP_BTN_LEFT] = 0;
	
	if(inp_keyPresses[INP_BTN_RIGHT] > 0 || (inp_keyHold[INP_BTN_RIGHT] && inp_keyTouched[INP_BTN_RIGHT])) {
		if(inp_buttonStates[INP_BTN_RIGHT] < 255) inp_buttonStates[INP_BTN_RIGHT]++;
	} else inp_buttonStates[INP_BTN_RIGHT] = 0;
	
	if(inp_keyPresses[INP_BTN_HOME] > 0 || (inp_keyHold[INP_BTN_HOME] && inp_keyTouched[INP_BTN_HOME])) {
		if(inp_buttonStates[INP_BTN_HOME] < 255) inp_buttonStates[INP_BTN_HOME]++;
	} else inp_buttonStates[INP_BTN_HOME] = 0;
	
	if(inp_keyPresses[INP_BTN_MENU] > 0 || (inp_keyHold[INP_BTN_MENU] && inp_keyTouched[INP_BTN_MENU])) {
		if(inp_buttonStates[INP_BTN_MENU] < 255) inp_buttonStates[INP_BTN_MENU]++;
	} else inp_buttonStates[INP_BTN_MENU] = 0;
	
	if(inp_keyPresses[INP_BTN_BACK] > 0 || (inp_keyHold[INP_BTN_BACK] && inp_keyTouched[INP_BTN_BACK])) {
		if(inp_buttonStates[INP_BTN_BACK] < 255) inp_buttonStates[INP_BTN_BACK]++;
	} else inp_buttonStates[INP_BTN_BACK] = 0;
	
	if(inp_keyPresses[INP_BTN_OK] > 0 || (inp_keyHold[INP_BTN_OK] && inp_keyTouched[INP_BTN_OK])) {
		if(inp_buttonStates[INP_BTN_OK] < 255) inp_buttonStates[INP_BTN_OK]++;
	} else inp_buttonStates[INP_BTN_OK] = 0;
	
	if(inp_keyPresses[INP_BTN_VOLUP] > 0 || (inp_keyHold[INP_BTN_VOLUP] && inp_keyTouched[INP_BTN_VOLUP])) {
		if(inp_buttonStates[INP_BTN_VOLUP] < 255) inp_buttonStates[INP_BTN_VOLUP]++;
	} else inp_buttonStates[INP_BTN_VOLUP] = 0;
	
	if(inp_keyPresses[INP_BTN_VOLDOWN] > 0 || (inp_keyHold[INP_BTN_VOLDOWN] && inp_keyTouched[INP_BTN_VOLDOWN])) {
		if(inp_buttonStates[INP_BTN_VOLDOWN] < 255) inp_buttonStates[INP_BTN_VOLDOWN]++;
	} else inp_buttonStates[INP_BTN_VOLDOWN] = 0;
	
	if(inp_keyPresses[INP_BTN_PWR] > 0 || (inp_keyHold[INP_BTN_PWR] && inp_keyTouched[INP_BTN_PWR])) {
		if(inp_buttonStates[INP_BTN_PWR] < 255) inp_buttonStates[INP_BTN_PWR]++;
	} else inp_buttonStates[INP_BTN_PWR] = 0;
}

// Gets the holding state of the given button
unsigned char inp_getButtonState(char button)
{
	if(button < 0 || button > (INP_BTN_TOTAL-1)) return 0;
	return inp_buttonStates[button];
}

// Sets the input device code for the given button
void inp_setButtonCode(char button, int code)
{
	if(button >= 0 && button < INP_BTN_TOTAL) {
		inp_keyCodes[button] = code;
	}
}

// Sets if the button should never be considered off
void inp_setButtonHold(char button, char hold)
{
	if(button >= 0 && button < INP_BTN_TOTAL) {
		inp_keyHold[button] = hold;
		inp_keyTouched[button] = 0;
	}
}

// Closes the Controller interface
int inp_close()
{
	inp_isInitFlag = 0;
	
	//wait for thread to stop
	if(inp_threadId > -1) pthread_join(inp_threadId, NULL);
	inp_threadId = -1;
	
	//close devices
	inp_clearDeviceList();
	
	return 0;
}

// Helper Functions
static void inp_checkButtonInput() {
	int i, n;
	
	//update list of input devices
	if((double)(clock() - inp_lastDeviceUpdate)/CLOCKS_PER_SEC > ((double)DEVICE_UPDATE_MILLIS)/1000.0) {
		inp_updateDeviceList();
	}
	
	//get input from devices
	for(i=0; i<inp_deviceSize; i++) {
		struct input_event ev[20];
		int rd = read(inp_deviceFd[i],ev,sizeof(ev));
		if(rd > 0) {
			int count = rd / sizeof(struct input_event);
			for(n=0; n<count; n++) {
				struct input_event *evp = &ev[n];
				//printf("type:%d, code:%d, value:%d\n", evp->type, evp->code, evp->value);
				if(evp->type == 1) {
					if(evp->code == inp_keyCodes[INP_BTN_UP]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_UP]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_UP]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_UP] = 1;
						if(inp_keyPresses[INP_BTN_UP] < 0) inp_keyPresses[INP_BTN_UP] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_DOWN]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_DOWN]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_DOWN]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_DOWN] = 1;
						if(inp_keyPresses[INP_BTN_DOWN] < 0) inp_keyPresses[INP_BTN_DOWN] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_LEFT]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_LEFT]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_LEFT]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_LEFT] = 1;
						if(inp_keyPresses[INP_BTN_LEFT] < 0) inp_keyPresses[INP_BTN_LEFT] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_RIGHT]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_RIGHT]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_RIGHT]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_RIGHT] = 1;
						if(inp_keyPresses[INP_BTN_RIGHT] < 0) inp_keyPresses[INP_BTN_RIGHT] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_HOME]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_HOME]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_HOME]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_HOME] = 1;
						if(inp_keyPresses[INP_BTN_HOME] < 0) inp_keyPresses[INP_BTN_HOME] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_MENU]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_MENU]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_MENU]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_MENU] = 1;
						if(inp_keyPresses[INP_BTN_MENU] < 0) inp_keyPresses[INP_BTN_MENU] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_BACK]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_BACK]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_BACK]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_BACK] = 1;
						if(inp_keyPresses[INP_BTN_BACK] < 0) inp_keyPresses[INP_BTN_BACK] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_OK]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_OK]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_OK]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_OK] = 1;
						if(inp_keyPresses[INP_BTN_OK] < 0) inp_keyPresses[INP_BTN_OK] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_VOLUP]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_VOLUP]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_VOLUP]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_VOLUP] = 1;
						if(inp_keyPresses[INP_BTN_VOLUP] < 0) inp_keyPresses[INP_BTN_VOLUP] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_VOLDOWN]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_VOLDOWN]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_VOLDOWN]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_VOLDOWN] = 1;
						if(inp_keyPresses[INP_BTN_VOLDOWN] < 0) inp_keyPresses[INP_BTN_VOLDOWN] = 0;
					} else if(evp->code == inp_keyCodes[INP_BTN_PWR]) {
						if(evp->value == 0) inp_keyPresses[INP_BTN_PWR]--;
						if(evp->value == 1) inp_keyPresses[INP_BTN_PWR]++;
						if(evp->value == 1) inp_keyTouched[INP_BTN_PWR] = 1;
						if(inp_keyPresses[INP_BTN_PWR] < 0) inp_keyPresses[INP_BTN_PWR] = 0;
					}
				}
			}
		}
    }
}
static int inp_addDeviceToList(char* path) {
	if(inp_deviceSize < MAX_DEVICES-1) {
		int fd = open(path, O_RDONLY | O_NONBLOCK);
		if(fd > -1) {
			inp_deviceFd[inp_deviceSize] = fd;
			inp_deviceName[inp_deviceSize] = (char*)malloc((strlen(path)+1)*sizeof(char));
			strcpy(inp_deviceName[inp_deviceSize], path);
			inp_deviceSize++;
			return inp_deviceSize-1;
		}
	}
	return -1;
}
static int inp_getDeviceFromList(char* path) {
	int i;
	for(i=0; i<inp_deviceSize; i++) {
		if(strcmp(path, inp_deviceName[i])==0) return i;
	}
	return -1;
}
static void inp_removeDeviceFromList(int index) {
	int i;
	if(index >= 0 && index < inp_deviceSize) {
		free(inp_deviceName[index]);
		close(inp_deviceFd[index]);
	}
	for(i=index; i<inp_deviceSize-1; i++) {
		inp_deviceName[i] = inp_deviceName[i+1];
		inp_deviceFd[i] = inp_deviceFd[i+1];
	}
	inp_deviceSize--;
}
static void inp_updateDeviceList() {
	int i;
	char deviceFoundFlag[MAX_DEVICES];
	for(i=0; i<MAX_DEVICES; i++) {
		if(i < inp_deviceSize) deviceFoundFlag[i] = 0;
		else deviceFoundFlag[i] = 1;
	}
	
	//open device list
	int devicesFd = open("/proc/bus/input/devices", O_RDONLY | O_NONBLOCK);
	if(devicesFd > 0) {
		FILE* fp = fdopen(devicesFd, "r");
		char line[1024];
		while(fgets(line, 1024, fp)!=NULL) {			
			if(strstr(line, "N: Name=")!=NULL) {
				char name[512];
				for(i=0; line[i+9]!='"' && line[i+9]!='\n'; i++) name[i] = line[i+9];
				name[i] = 0;
				
				//check for new device
				while(fgets(line, 1024, fp)!=NULL) {
					char* ev;
					if(strstr(line, "H: Handlers=")!=NULL) {
						if((ev=strstr(line, "event"))!=NULL && (strstr(line, "kbd")!=NULL || strstr(line, "js0")!=NULL || strstr(line, "js1")!=NULL)) {
							char event[64];
							for(i=0; ev[i]!=' ' && ev[i]!='\n'; i++) event[i] = ev[i];
							event[i] = 0;
							char fullPath[1024];
							sprintf(fullPath, "/dev/input/%s", event);
							
							int deviceIndex = inp_getDeviceFromList(fullPath);
							if(deviceIndex > -1) deviceFoundFlag[deviceIndex] = 1;
							else {
								//printf("found device %s: %s\n", name, fullPath); //new device
								inp_addDeviceToList(fullPath);
							}
						}
						break;
					}
				}
			}
		}
		fclose(fp);
	}
	
	//remove devices that no longer exist
	for(i=inp_deviceSize-1; i>=0; i--) {
		if(deviceFoundFlag[i]==0) {
			//printf("lost device %s\n", inp_deviceName[i]); //lost device
			inp_removeDeviceFromList(i);
		}
	}
	
	inp_lastDeviceUpdate = clock();
}
static void inp_clearDeviceList() {
	int i;
	for(i=0; i<inp_deviceSize; i++) {
		free(inp_deviceName[i]);
		close(inp_deviceFd[i]);
	}
	inp_deviceSize = 0;
}
