#ifndef LED_H
#define LED_H

// Configuration options for panel matrix style displays
struct LEDPanelOptions {
	char hardware_mapping[32];      /* Name of GPIO mapping used (default "regular") */
	int rows;                       /* Panel rows (typically 8, 16, 32 or 64; default 32) */
	int cols;                       /* Panel columns (typically 32 or 64; default 32) */
	int chain_length;               /* Number of daisy-chained panels (default 1) */
	int parallel;                   /* Parallel chains (range 1..3; default 1) */
	int pwm_bits;                   /* PWM bits (range 1..11; default 11) */
	int pwm_lsb_nanoseconds;        /* PWM nanoseconds for LSB (default 130) */
	int pwm_dither_bits;            /* Time dithering of lower bits (range 0..2; default 0) */
	int scan_mode;                  /* Scan mode (0=progressive, 1=interlaced; default 0) */
	int row_address_type;           /* Row address type (0=direct, 1=AB-addressed panels, 2=direct row select, 3=ABC-addressed panels; default 0) */
	int multiplexing;               /* Mux type (0=direct, 1=stripe, 2=checkered, 3=spiral, 4=ZStripe, 5=ZnMirrorZStripe, 6=coreman, 7=Kaler2Scan, 8=ZStripeUneven, 9=P10-128x4-Z, 10=QiangLiQ8; default 0) */
	char led_rgb_sequence[8];       /* The "RGB" sequence (default "RGB") */
	char pixel_mapper_config[128];  /* Semicolon-separated list of pixel-mappers with optional parameter (default "") */
	char panel_type[16];            /* Needed to initialize special panels (supported "FM6126A"; default "") */
	char disable_hardware_pulsing;  /* Flag to not use hardware pin-pulse generation (default 0) */
	char inverse_colors;            /* Flag to invert colors (default 0) */
	int mirror_x;                   /* Flag to mirror on the x axis (default 0) */
	int mirror_y;                   /* Flag to mirror on the y axis (default 0) */
};

// Configuration options for panel matrix style displays
struct LEDStripOptions {
	int dimension_x;                /* X dimension of the strip (default 32) */
	int dimension_y;                /* Y dimension of the strip (default 16) */
	int skip;                       /* Number of leds to skip over (default 0) */
	char layout[4];                 /* Strip layout ("XY" vs "YX"; default "XY") */
	char rgb_sequence[8];           /* The "RGB(W)" sequence (default "GRB") */
	int frequency;                  /* The target data frequency (default 800000) */
	int invert;                     /* Flag to invert colors (default 0) */
	int mirror_x;                   /* Flag to mirror on the x axis (default 0) */
	int mirror_y;                   /* Flag to mirror on the y axis (default 0) */
};


// Setup and initialize the LED Display utils
int led_init(struct LEDPanelOptions* panelOptions, struct LEDStripOptions* stripOptions);

// Checks if the LED Display utils are initialized
char led_isInit();

// Fills the LED Display buffer with the given color
void led_fill(unsigned char red, unsigned char green, unsigned char blue);

// Writes a color value to the LED Display buffer at the given position
void led_write(int x, int y, unsigned char red, unsigned char green, unsigned char blue);

// Gets the brightness of the LED Display [1-100]
char led_getBrightness();

// Sets the brightness of the LED Display [1-100]
void led_setBrightness(char bright);

// Sends LED Display buffer data to the display
void led_flush();

// Sets LEDPanelOptions defaults
void led_setPanelDefaults(struct LEDPanelOptions* options);

// Sets LEDStripOptions defaults
void led_setStripDefaults(struct LEDStripOptions* options);

// Cleans up the LED Display utils
int led_close();

#endif /* LED_H */
