#include "led.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

//defines
#define	SPI_CHAN		0
#define	SPI_SPEED		2000000
#define SPI_PIN_CLK 	14
#define SPI_PIN_DAT 	12

#define LED_STATE_CLOSED   0
#define LED_STATE_IDLE     1
#define LED_STATE_READY    2
#define LED_STATE_DRAWING  3
#define LED_STATE_STOPPING 4
#define LED_STATE_STOPPED  5

#define LED_BUFFER_SIZE (LED_DIMENSION_X*LED_DIMENSION_Y*LED_DIMENSION_Z*3)
#define LED_MAX_FPS     60
#define LED_DEF_BRIGHT  100

#define META_NUM_BYTES  2
#define META_SYNC_BYTE  0x01
#define META_RESET      0x20
#define META_STANDBY    0x10

//data
static unsigned char led_dataBuffers[2][LED_BUFFER_SIZE+META_NUM_BYTES];
static unsigned char *led_spiBufferWorking = 0;
static unsigned char *led_spiBufferTransfer = 0;
static unsigned char led_state = LED_STATE_CLOSED;
static unsigned long led_lastTransferTimestamp = 0;
static int led_spiFd = -1;
static unsigned char led_brightness = LED_DEF_BRIGHT;
static unsigned char led_metaReset = 0;
static unsigned char led_metaStandby = 0;

//helper functions
unsigned long led_getTimestamp();

// Transfer thread
static pthread_t led_transferThread;
void *led_transferLoop(void *args) {
	int i;
	while(led_state == LED_STATE_IDLE || led_state == LED_STATE_READY) {
		if(led_state == LED_STATE_READY) {
			led_state = LED_STATE_DRAWING;
			unsigned char spiBuffer[LED_BUFFER_SIZE+META_NUM_BYTES];
			for(i=0; i<LED_BUFFER_SIZE; i++) spiBuffer[i] = led_spiBufferTransfer[i];
			
			unsigned char metaByte = 0x00;
			if(led_brightness > 75) metaByte = 0x03;
			else if(led_brightness > 50) metaByte = 0x02;
			else if(led_brightness > 25) metaByte = 0x01;
			if(led_metaReset) metaByte += META_RESET;
			else if(led_metaStandby) metaByte += META_STANDBY;
			spiBuffer[LED_BUFFER_SIZE+0] = metaByte;
			spiBuffer[LED_BUFFER_SIZE+1] = META_SYNC_BYTE;
			
			wiringPiSPIDataRW(SPI_CHAN, spiBuffer, LED_BUFFER_SIZE+META_NUM_BYTES);
			
			if(led_state == LED_STATE_DRAWING) led_state = LED_STATE_IDLE;
			led_metaReset = 0;
			led_metaStandby = 0;
		} else {
			usleep(100);
		}
	}
	
	led_state = LED_STATE_STOPPED;
	pthread_exit(NULL);
}

// Setup and initialize the LED Display utils
int led_init()
{
	int i;
	
	//clear data
	for(i=0; i<LED_BUFFER_SIZE; i++) {
		led_dataBuffers[0][i] = 0;
		led_dataBuffers[1][i] = 0;
	}
	led_spiBufferWorking = led_dataBuffers[0];
	led_spiBufferTransfer = led_dataBuffers[1];
	led_lastTransferTimestamp = 0;
	led_state = LED_STATE_CLOSED;
	
	//setup wiringpi
	wiringPiSetup();
	if((led_spiFd = wiringPiSPISetup(SPI_CHAN, SPI_SPEED)) < 0) {
		fprintf(stderr, "led_init: Failed to open SPI bus: %s\n", strerror(errno));
		led_close();
		return 1;
	}
	
	//startup thread
	if(pthread_create(&led_transferThread, NULL, led_transferLoop, 0) < 0) {
		fprintf(stderr, "led_init: Failed to create output thread: %s\n", strerror(errno));
		led_close();
		return 1;
	}
	
	led_state = LED_STATE_IDLE;
	return 0;
}

// Checks if the LED Display utils are initialized
char led_isInit()
{
	if(led_state != LED_STATE_CLOSED) return 1;
	return 0;
}

// Fills the LED Display buffer with the given color
void led_fill(unsigned char red, unsigned char green, unsigned char blue)
{
	int x,y,z;
	if(led_state != LED_STATE_CLOSED) {
		for(x=0; x<LED_DIMENSION_X; x++) {
			for(y=0; y<LED_DIMENSION_Y; y++) {
				for(z=0; z<LED_DIMENSION_Z; z++) {
					led_write(x, y, z, red, green, blue);
				}
			}
		}
	}
}

// Writes a color value to the LED Display buffer at the given position
void led_write(int x, int y, int z, unsigned char red, unsigned char green, unsigned char blue)
{
	if(led_state != LED_STATE_CLOSED && x>=0 && x<LED_DIMENSION_X && y>=0 && y<LED_DIMENSION_Y && z>=0 && z<LED_DIMENSION_Z) {
		led_spiBufferWorking[(y*LED_DIMENSION_Z*LED_DIMENSION_X + z*LED_DIMENSION_X + x)*3 + 0] = red;
		led_spiBufferWorking[(y*LED_DIMENSION_Z*LED_DIMENSION_X + z*LED_DIMENSION_X + x)*3 + 1] = green;
		led_spiBufferWorking[(y*LED_DIMENSION_Z*LED_DIMENSION_X + z*LED_DIMENSION_X + x)*3 + 2] = blue;
	}
}

// Gets the brightness of the LED Display [1-100]
char led_getBrightness()
{
	return led_brightness;
}

// Sets the brightness of the LED Display [1-100]
void led_setBrightness(char bright)
{
	if(bright < 1) bright = 1;
	if(bright > 100) bright = 100;
	
	led_brightness = bright;
}

// Sends LED Display buffer data to the display
void led_flush()
{
	int i;
	
	//make sure to not cross given highest FPS
	long minTime = 1000000/LED_MAX_FPS;
	long timeSinceLastTransfer = led_getTimestamp() - led_lastTransferTimestamp;
	if(timeSinceLastTransfer > 0 && led_lastTransferTimestamp > 0) {
		if(timeSinceLastTransfer < minTime) usleep(minTime - timeSinceLastTransfer);
	} else {
		usleep(minTime);
	}
	led_lastTransferTimestamp = led_getTimestamp();
	
	//switch the buffers and flag the state to draw
	for(i=0; i<LED_BUFFER_SIZE; i++) led_spiBufferTransfer[i] = led_spiBufferWorking[i];
	unsigned char *temp = led_spiBufferWorking;
	led_spiBufferWorking = led_spiBufferTransfer;
	led_spiBufferTransfer = temp;
	led_state = LED_STATE_READY;
}

// Sets the LED Display into reset mode
void led_reset()
{
	//make sure to not cross given highest FPS
	long minTime = 1000000/LED_MAX_FPS;
	usleep(minTime);
	
	//flag the state to draw and wait till finished (x2)
	led_metaReset = 1;
	led_state = LED_STATE_READY;
	while(led_state != LED_STATE_IDLE) usleep(1000);
	led_metaReset = 1;
	led_state = LED_STATE_READY;
	while(led_state != LED_STATE_IDLE) usleep(1000);
}

// Sets the LED Display into standby mode
void led_standby()
{
	//make sure to not cross given highest FPS
	long minTime = 1000000/LED_MAX_FPS;
	usleep(minTime);
	
	//flag the state to draw and wait till finished (x2)
	led_metaStandby = 1;
	led_state = LED_STATE_READY;
	while(led_state != LED_STATE_IDLE) usleep(1000);
	led_metaStandby = 1;
	led_state = LED_STATE_READY;
	while(led_state != LED_STATE_IDLE) usleep(1000);
	
	//set clock line high
	pinMode(SPI_PIN_CLK, OUTPUT);
	digitalWrite(SPI_PIN_CLK, HIGH);
}

// Cleans up the LED Display utils
int led_close()
{
	//close the thread
	if(led_state == LED_STATE_IDLE || led_state == LED_STATE_READY || led_state == LED_STATE_DRAWING) {
		while(led_state != LED_STATE_IDLE) usleep(1000);
		
		led_state = LED_STATE_STOPPING;
		while(led_state != LED_STATE_STOPPED) usleep(1000);
	}
	
	//close SPI
	if(led_spiFd >= 0) close(led_spiFd);	
	led_spiFd = -1;
	
	led_state = LED_STATE_CLOSED;
	return 0;
}

//helper functions
unsigned long led_getTimestamp() {
	struct timeval currentTime;
	if(gettimeofday(&currentTime, NULL) > -1) {
		return (((unsigned long)currentTime.tv_sec-(((unsigned long)currentTime.tv_sec/1000)*1000))*1000000) + (unsigned long)currentTime.tv_usec;
	}
	return 0;
}
