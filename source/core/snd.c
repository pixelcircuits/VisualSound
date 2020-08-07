#include "snd.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <alsa/asoundlib.h>

#define DEVICE_PCM_LATENCY 100000   /* Latency for ring and pcm buffers in us (overall latency will be ~2x this) */
#define DEVICE_PCM_RATE 48000       /* Sample rate for pcm buffers */
#define DEVICE_PCM_CHANNELS 2       /* Number of channels for pcm buffers */

#define MASTER_BUFFER_SIZE 24000                                                      /* Approx num of samples in the master buffer */
#define MASTER_BUFFER_PERIOD (((DEVICE_PCM_RATE*(DEVICE_PCM_LATENCY/1000))/1000)/8)   /* The number of samples in a segment of the buffer (1/8 the latency period) */
#define MASTER_BUFFER_SEGMENTS ((MASTER_BUFFER_SIZE/MASTER_BUFFER_PERIOD)+1)          /* The total number of segments in the buffer */

#define THREAD_STATUS_RUNNING 0
#define THREAD_STATUS_CLOSING 1
#define THREAD_STATUS_END 2

//constants
static const char* snd_deviceDefault = "hw:1,0";
static const char* snd_deviceVolumeCommand = "amixer -c 1 set 'Speaker' %s";

//data
static const char* snd_inputDeviceName;
static const char* snd_outputDeviceName;
static unsigned char snd_volume;
static signed short snd_sampleBuffer[MASTER_BUFFER_SEGMENTS*MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS];   /* The full buffer where all sound data is recorded */
static signed short* snd_masterRingBuffer[MASTER_BUFFER_SEGMENTS];                                       /* Pointers to the full buffer to help denote sample order */

//logic thread
static char snd_processSoundThreadStatus = THREAD_STATUS_END;
static void* snd_processSound(void* args);
static void snd_startSoundThread();
static void snd_stopSoundThread();

//helper functions
static snd_pcm_t* snd_getInputPCM(const char* name);
static snd_pcm_t* snd_getOutputPCM(const char* name);
static int snd_writePCM(snd_pcm_t* pcm, signed short* buffer, unsigned int numFrames);
static int snd_readPCM(snd_pcm_t* pcm, signed short* buffer, unsigned int numFrames);
static signed short* snd_rotateRingBuffer(signed short** buffer, unsigned int numSegments);
static signed short snd_readRingBuffer(signed short** buffer, unsigned int numSegments, unsigned int segmentSize, unsigned int index);
static void snd_usleep(long useconds);

// Setup and initialize the Sound utils
int snd_init(const char* outputDevice)
{
	//data
	int i;
	for(i=0; i<MASTER_BUFFER_SEGMENTS*MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS; i++) snd_sampleBuffer[i] = 0;
	for(i=0; i<MASTER_BUFFER_SEGMENTS; i++) snd_masterRingBuffer[i] = &(snd_sampleBuffer[i*MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS]);
	snd_inputDeviceName = 0;
	snd_outputDeviceName = 0;
	
	snd_setOutputDevice(outputDevice);
	snd_setVolume(80);
	return 0;
}

// Checks if the Sound utils are initialized
char snd_isInit()
{
	return 1;
}

// Sets the input device
void snd_setInputDevice(const char* inputDevice)
{
	snd_stopSoundThread();
	snd_inputDeviceName = inputDevice;
	snd_startSoundThread();
}

// Sets the output device
void snd_setOutputDevice(const char* outputDevice)
{
	if(!outputDevice) outputDevice = snd_deviceDefault;
	snd_stopSoundThread();
	snd_outputDeviceName = outputDevice;
	snd_startSoundThread();
}

// Gets the rate of the sound buffer
unsigned int snd_getBufferRate()
{
	return DEVICE_PCM_RATE;
}

// Gets if the sound is running
char snd_getIsRunning()
{
	if(snd_processSoundThreadStatus == THREAD_STATUS_RUNNING) return 1;
	return 0;
}

// Fills the given buffer with data from the sound buffer
void snd_collectSamples(signed short* buffer, unsigned int sampleRate, unsigned int numSamples)
{
	int i;
	if(snd_processSoundThreadStatus != THREAD_STATUS_RUNNING) {
		for(i=0; i<numSamples; i++) buffer[i] = 0;
		return;
	}
	
	int segmentSize = MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS;
	int skip = (DEVICE_PCM_RATE/sampleRate);
	signed short* ringBuffer[MASTER_BUFFER_SEGMENTS];
	for(i=0; i<MASTER_BUFFER_SEGMENTS; i++) ringBuffer[i] = snd_masterRingBuffer[i];
	
	//collect samples
	int index = (MASTER_BUFFER_SEGMENTS*segmentSize) - (numSamples*skip*DEVICE_PCM_CHANNELS);
	for(i=0; i<numSamples; i+=2) {
		buffer[i] = snd_readRingBuffer(ringBuffer, MASTER_BUFFER_SEGMENTS, segmentSize, index);
		if(DEVICE_PCM_CHANNELS == 2) buffer[i+1] = snd_readRingBuffer(ringBuffer, MASTER_BUFFER_SEGMENTS, segmentSize, index+1);
		else buffer[i+1] = buffer[i];
	
		index += skip*DEVICE_PCM_CHANNELS;
	}
}

// Plays the given sound file
void snd_playFile(const char* filename)
{
	char command[1024];
	if(snd_outputDeviceName) sprintf(command, "aplay -q %s -D %s", filename, snd_outputDeviceName);
	else sprintf(command, "aplay -q %s", filename);
	system(command);
}

// Sets the system volume [0-100]
void snd_setVolume(char volume)
{
	if(volume > 100) volume = 100;
	if(volume < 0) volume = 0;
	snd_volume = volume;
	
	if(snd_outputDeviceName) {
		char suffix[64];
		sprintf(suffix, "%d%% > /dev/null 2>&1", volume);
		char command[128];
		sprintf(command, snd_deviceVolumeCommand, suffix);
		system(command);
	}
}

// Gets the system volume [0-100]
unsigned char snd_getVolume()
{
	return snd_volume;
}

// Cleans up the Sound utils
int snd_close()
{
	snd_stopSoundThread();
	return 0;
}

// Sound Proccessing Thread
static void* snd_processSound(void* args)
{
	int i, err;
	if(!snd_inputDeviceName || !snd_outputDeviceName) {
		snd_processSoundThreadStatus = THREAD_STATUS_END;
		return 0;
	}
	
	//setup devices
	snd_pcm_t* snd_inputHandle = snd_getInputPCM(snd_inputDeviceName);
	snd_pcm_t* snd_outputHandle = snd_getOutputPCM(snd_outputDeviceName);
	if(!snd_inputHandle || !snd_outputHandle) {
		if(snd_inputHandle) snd_pcm_close(snd_inputHandle);
		if(snd_outputHandle) snd_pcm_close(snd_outputHandle);
		snd_processSoundThreadStatus = THREAD_STATUS_END;
		return 0;
	}
	
	//setup data
	for(i=0; i<MASTER_BUFFER_SEGMENTS*MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS; i++) snd_sampleBuffer[i] = 0;
	for(i=0; i<MASTER_BUFFER_SEGMENTS; i++) snd_masterRingBuffer[i] = &(snd_sampleBuffer[i*MASTER_BUFFER_PERIOD*DEVICE_PCM_CHANNELS]);
	
	//start by giving the output buffer a head start
	snd_writePCM(snd_outputHandle, snd_masterRingBuffer[0], MASTER_BUFFER_PERIOD);
	snd_writePCM(snd_outputHandle, snd_masterRingBuffer[0], MASTER_BUFFER_PERIOD);
	
	//pass data from the input device to the output device
	while(1==1) {
		if(snd_processSoundThreadStatus) break;
		signed short* buffer = snd_rotateRingBuffer(snd_masterRingBuffer, MASTER_BUFFER_SEGMENTS);
		
		while(!snd_processSoundThreadStatus && snd_pcm_avail(snd_inputHandle) < MASTER_BUFFER_PERIOD) snd_usleep(10000);
		if(snd_processSoundThreadStatus) break;
		
		err = snd_readPCM(snd_inputHandle, buffer, MASTER_BUFFER_PERIOD);
		if(err < 0) break;
		
		if(snd_processSoundThreadStatus) break;
		
		err = snd_writePCM(snd_outputHandle, buffer, MASTER_BUFFER_PERIOD);
		if(err < 0) break;
	}
	
	//cleanup and exit
	snd_pcm_close(snd_inputHandle);
	snd_pcm_close(snd_outputHandle);
	snd_processSoundThreadStatus = THREAD_STATUS_END;
	return 0;
}
void snd_startSoundThread()
{
	pthread_t threadId;
	snd_stopSoundThread();
	
	snd_processSoundThreadStatus = THREAD_STATUS_RUNNING;
    pthread_create(&threadId, NULL, snd_processSound, NULL);
}
void snd_stopSoundThread()
{
	if(snd_processSoundThreadStatus == THREAD_STATUS_RUNNING) snd_processSoundThreadStatus = THREAD_STATUS_CLOSING;
	while(snd_processSoundThreadStatus != THREAD_STATUS_END) snd_usleep(10000);
}

//helper functions
static snd_pcm_t* snd_getInputPCM(const char* name) {
	int err;
	snd_pcm_t* pcm;
	if((err = snd_pcm_open(&pcm, name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		printf("[SND] Capture open error: %s\n", snd_strerror(err));
		return 0;
	}
	if((err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, DEVICE_PCM_CHANNELS, DEVICE_PCM_RATE, 1, DEVICE_PCM_LATENCY)) < 0) {
		if(pcm) snd_pcm_close(pcm);
		printf("[SND] Capture param error: %s\n", snd_strerror(err));
		return 0;
	}
	snd_pcm_prepare(pcm);
	snd_pcm_start(pcm);
	return pcm;
}
static snd_pcm_t* snd_getOutputPCM(const char* name) {
	int err;
	snd_pcm_t* pcm;
	if((err = snd_pcm_open(&pcm, name, SND_PCM_STREAM_PLAYBACK , 0)) < 0) {
		printf("[SND] Playback open error: %s\n", snd_strerror(err));
		return 0;
	}
	if((err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, DEVICE_PCM_CHANNELS, DEVICE_PCM_RATE, 1, DEVICE_PCM_LATENCY)) < 0) {
		if(pcm) snd_pcm_close(pcm);
		printf("[SND] Playback param error: %s\n", snd_strerror(err));
		return 0;
	}
	snd_pcm_prepare(pcm);
	snd_pcm_start(pcm);
	return pcm;
}
static int snd_writePCM(snd_pcm_t* pcm, signed short* buffer, unsigned int numFrames) {
	snd_pcm_sframes_t frames = snd_pcm_writei(pcm, buffer, numFrames);
	if(frames < 0) frames = snd_pcm_recover(pcm, frames, 0);
	if(frames < 0) printf("[SND] snd_pcm_writei failed: %d %s\n", frames, snd_strerror(frames));
	if(frames > 0 && frames < numFrames) printf("[SND] Short write (expected %li, wrote %li)\n", numFrames, frames);
	return frames;
}
static int snd_readPCM(snd_pcm_t* pcm, signed short* buffer, unsigned int numFrames) {
	snd_pcm_sframes_t frames = snd_pcm_readi(pcm, buffer, numFrames);
	if(frames < 0) frames = snd_pcm_recover(pcm, frames, 0);
	if(frames < 0) printf("[SND] snd_pcm_readi failed: %d %s\n", frames, snd_strerror(frames));
	if(frames > 0 && frames < numFrames) printf("[SND] Short read (expected %li, read %li)\n", numFrames, frames);
	return frames;
}
static signed short* snd_rotateRingBuffer(signed short** buffer, unsigned int numSegments) {
	int i;
	signed short* temp = buffer[0];
	for(i=0; i<numSegments-1; i++) buffer[i] = buffer[i+1];
	buffer[numSegments-1] = temp;
	
	return temp;
}
static signed short snd_readRingBuffer(signed short** buffer, unsigned int numSegments, unsigned int segmentSize, unsigned int index) {
	unsigned int segmentIndex = index/segmentSize;
	unsigned int indexInSegment = index - (segmentIndex*segmentSize);
	if(segmentIndex >= numSegments) return 0;
	return buffer[segmentIndex][indexInSegment];
}
static void snd_usleep(long useconds) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = useconds * 1000;
	nanosleep(&ts, &ts);
}
