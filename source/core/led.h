#ifndef LED_H
#define LED_H

#define LED_DIMENSION_X 10
#define LED_DIMENSION_Y 10
#define LED_DIMENSION_Z 10

// Setup and initialize the LED Display utils
int led_init();

// Checks if the LED Display utils are initialized
char led_isInit();

// Fills the LED Display buffer with the given color
void led_fill(unsigned char red, unsigned char green, unsigned char blue);

// Writes a color value to the LED Display buffer at the given position
void led_write(int x, int y, int z, unsigned char red, unsigned char green, unsigned char blue);

// Gets the brightness of the LED Display [1-100]
char led_getBrightness();

// Sets the brightness of the LED Display [1-100]
void led_setBrightness(char bright);

// Sends LED Display buffer data to the display
void led_flush();

// Sets the LED Display into reset mode
void led_reset();

// Sets the LED Display into standby mode
void led_standby();

// Cleans up the LED Display utils
int led_close();

#endif /* LED_H */