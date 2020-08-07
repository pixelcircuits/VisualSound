#include "led.h"
#include <led-matrix-c.h>
#include <ws2811.h>
#include <string.h>

#define DRIVER_MODE_NONE 0
#define DRIVER_MODE_PANEL 1
#define DRIVER_MODE_STRIP 2

//data
static int led_driverMode = DRIVER_MODE_NONE;

//panel driver data
static struct RGBLedMatrix* led_matrix = NULL;
static struct LedCanvas* led_canvas = NULL;
static struct LEDPanelOptions led_panelOptions;

//strip driver data
ws2811_t led_string;
static struct LEDStripOptions led_stripOptions;

// Setup and initialize the LED Display utils
int led_init(struct LEDPanelOptions* panelOptions, struct LEDStripOptions* stripOptions)
{
	int i;
	led_setPanelDefaults(&led_panelOptions);
	led_setStripDefaults(&led_stripOptions);
	if(panelOptions) {
		memcpy(&led_panelOptions, panelOptions, sizeof(led_panelOptions));
		led_driverMode = DRIVER_MODE_PANEL;
		
		//load panel based driver
		struct RGBLedMatrixOptions options;
		memset(&options, 0, sizeof(options));
		options.hardware_mapping = led_panelOptions.hardware_mapping;
		options.rows = led_panelOptions.rows;
		options.cols = led_panelOptions.cols;
		options.chain_length = led_panelOptions.chain_length;
		options.parallel = led_panelOptions.parallel;
		options.pwm_bits = led_panelOptions.pwm_bits;
		options.pwm_lsb_nanoseconds = led_panelOptions.pwm_lsb_nanoseconds;
		options.pwm_dither_bits = led_panelOptions.pwm_dither_bits;
		options.scan_mode = led_panelOptions.scan_mode;
		options.row_address_type = led_panelOptions.row_address_type;
		options.multiplexing = led_panelOptions.multiplexing;
		options.led_rgb_sequence = led_panelOptions.led_rgb_sequence;
		options.pixel_mapper_config = led_panelOptions.pixel_mapper_config;
		options.panel_type = led_panelOptions.panel_type;
		options.disable_hardware_pulsing = led_panelOptions.disable_hardware_pulsing;
		options.inverse_colors = led_panelOptions.inverse_colors;
		options.brightness = 100;
		led_matrix = led_matrix_create_from_options(&options, NULL, NULL);
		if(led_matrix == NULL) {
			fprintf(stderr, "led_init: Failed to setup rgb led matrix\n");
			led_close();
			return 1;
		}
		led_canvas = led_matrix_create_offscreen_canvas(led_matrix);
		
	} else if(stripOptions) {
		memcpy(&led_stripOptions, stripOptions, sizeof(led_stripOptions));
		led_driverMode = DRIVER_MODE_STRIP;
		
		//load strip based driver
		int stripType = WS2811_STRIP_GRB;
		if(strcmp(led_stripOptions.rgb_sequence, "RGBW")==0) stripType = SK6812_STRIP_RGBW;
		else if(strcmp(led_stripOptions.rgb_sequence, "RBGW")==0) stripType = SK6812_STRIP_RBGW;
		else if(strcmp(led_stripOptions.rgb_sequence, "GRBW")==0) stripType = SK6812_STRIP_GRBW;
		else if(strcmp(led_stripOptions.rgb_sequence, "GBRW")==0) stripType = SK6812_STRIP_GBRW;
		else if(strcmp(led_stripOptions.rgb_sequence, "BRGW")==0) stripType = SK6812_STRIP_BRGW;
		else if(strcmp(led_stripOptions.rgb_sequence, "BGRW")==0) stripType = SK6812_STRIP_BGRW;
		else if(strcmp(led_stripOptions.rgb_sequence, "RGB")==0) stripType = WS2811_STRIP_RGB;
		else if(strcmp(led_stripOptions.rgb_sequence, "RBG")==0) stripType = WS2811_STRIP_RBG;
		else if(strcmp(led_stripOptions.rgb_sequence, "GRB")==0) stripType = WS2811_STRIP_GRB;
		else if(strcmp(led_stripOptions.rgb_sequence, "GBR")==0) stripType = WS2811_STRIP_GBR;
		else if(strcmp(led_stripOptions.rgb_sequence, "BRG")==0) stripType = WS2811_STRIP_BRG;
		else if(strcmp(led_stripOptions.rgb_sequence, "BGR")==0) stripType = WS2811_STRIP_BGR;
		led_string.freq = led_stripOptions.frequency;
		led_string.dmanum = 10;
		led_string.channel[0].gpionum = 18;
		led_string.channel[0].count = (led_stripOptions.dimension_x*led_stripOptions.dimension_y)+1;
		led_string.channel[0].invert = led_stripOptions.invert;
		led_string.channel[0].brightness = 255;
		led_string.channel[0].strip_type = stripType;
		led_string.channel[1].gpionum = 0;
		led_string.channel[1].count = 0;
		led_string.channel[1].invert = 0;
		led_string.channel[1].brightness = 0;
		ws2811_return_t ret;
		if((ret = ws2811_init(&led_string)) != WS2811_SUCCESS) {
			fprintf(stderr, "led_init: Failed to setup rgb led strip: %s\n", ws2811_get_return_t_str(ret));
			led_close();
			return 1;
		}
	}
	return 0;
}

// Checks if the LED Display utils are initialized
char led_isInit()
{
	if(led_driverMode != DRIVER_MODE_NONE) return 1;
	return 0;
}

// Fills the LED Display buffer with the given color
void led_fill(unsigned char red, unsigned char green, unsigned char blue)
{
	if(led_driverMode == DRIVER_MODE_PANEL) {
		led_canvas_fill(led_canvas, red, green, blue);
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		int x,y;
		for(x=0; x<led_stripOptions.dimension_x; x++) {
			for(y=0; y<led_stripOptions.dimension_y; y++) {
				led_write(x, y, red, green, blue);
			}
		}
	}
}

// Writes a color value to the LED Display buffer at the given position
void led_write(int x, int y, unsigned char red, unsigned char green, unsigned char blue)
{
	//mirror and dimensions
	int dimension_x = 0;
	int dimension_y = 0;
	if(led_driverMode == DRIVER_MODE_PANEL) {
		dimension_x = led_panelOptions.cols*led_panelOptions.chain_length;
		dimension_y = led_panelOptions.rows*led_panelOptions.parallel;
		if(led_panelOptions.mirror_x > 0) x = (dimension_x-1) - x;
		if(led_panelOptions.mirror_y > 0) y = (dimension_y-1) - y;
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		dimension_x = led_stripOptions.dimension_x;
		dimension_y = led_stripOptions.dimension_y;
		if(led_stripOptions.mirror_x > 0) x = (dimension_x-1) - x;
		if(led_stripOptions.mirror_y > 0) y = (dimension_y-1) - y;
	}
	
	//draw
	if(x < dimension_x && x >= 0 && y < dimension_y && y >= 0) {
		if(led_driverMode == DRIVER_MODE_PANEL) {
			led_canvas_set_pixel(led_canvas, x, y, red, green, blue);
		} else if(led_driverMode == DRIVER_MODE_STRIP) {
			int n=led_stripOptions.skip;
			if(strcmp(led_stripOptions.layout, "YX")==0) {
				if((y%2)==1) x = (led_stripOptions.dimension_x-1) - x;
				n += y*led_stripOptions.dimension_x + x;
			} else if(strcmp(led_stripOptions.layout, "XY")==0) {
				if((x%2)==1) y = (led_stripOptions.dimension_y-1) - y;
				n += x*led_stripOptions.dimension_y + y;
			}
			int color = (int)(red) << 16 + (int)(green) << 8 + (int)(blue) << 0;
			led_string.channel[0].leds[n] = color;
		}
	}
}

// Gets the brightness of the LED Display [1-100]
char led_getBrightness()
{
	if(led_driverMode == DRIVER_MODE_PANEL) {
		return led_matrix_get_brightness(led_matrix);
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		return ((float)led_string.channel[0].brightness) / 2.55f;
	}
	return 0;
}

// Sets the brightness of the LED Display [1-100]
void led_setBrightness(char bright)
{
	if(bright > 100) bright = 100;
	if(bright < 1) bright = 1;
	if(led_driverMode == DRIVER_MODE_PANEL) {
		led_matrix_set_brightness(led_matrix, bright);
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		led_string.channel[0].brightness = ((float)bright)*2.55f;
	}
}

// Sends LED Display buffer data to the display
void led_flush()
{
	if(led_driverMode == DRIVER_MODE_PANEL) {
		led_canvas = led_matrix_swap_on_vsync(led_matrix, led_canvas);
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		ws2811_render(&led_string);
	}
}

// Sets LEDPanelOptions defaults
void led_setPanelDefaults(LEDPanelOptions* options)
{
	options->hardware_mapping[31] = 0;
	strncpy(options->hardware_mapping, "regular", 31);
	options->rows = 32;
	options->cols = 32;
	options->chain_length = 1;
	options->parallel = 1;
	options->pwm_bits = 11;
	options->pwm_lsb_nanoseconds = 130;
	options->pwm_dither_bits = 0;
	options->scan_mode = 0;
	options->row_address_type = 0;
	options->multiplexing = 0;
	options->led_rgb_sequence[7] = 0;
	strncpy(options->led_rgb_sequence, "RGB", 7);
	options->pixel_mapper_config[0] = 0;
	options->panel_type[0] = 0;
	options->disable_hardware_pulsing = 0;
	options->inverse_colors = 0;
	options->mirror_x = 0;
	options->mirror_y = 0;
}

// Sets LEDStripOptions defaults
void led_setStripDefaults(struct LEDStripOptions* options)
{
	options->dimension_x = 32;
	options->dimension_y = 16;
	options->skip = 0;
	options->layout[3] = 0;
	strncpy(options->layout, "XY", 3);
	options->rgb_sequence[7] = 0;
	strncpy(options->rgb_sequence, "RGB", 7);
	options->frequency = 800000;
	options->invert = 0;
	options->mirror_x = 0;
	options->mirror_y = 0;
}

// Cleans up the LED Display utils
int led_close()
{
	if(led_driverMode == DRIVER_MODE_PANEL) {
		if(led_matrix) led_matrix_delete(led_matrix);
		led_matrix = NULL;
		led_canvas = NULL;
	} else if(led_driverMode == DRIVER_MODE_STRIP) {
		ws2811_fini(&led_string);
	}
	led_driverMode = DRIVER_MODE_NONE;
	return 0;
}
