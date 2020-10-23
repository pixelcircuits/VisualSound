//-----------------------------------------------------------------------------------------
// Title:	Music and Sound Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "led.h"
#include "inp.h"
#include "bt.h"
#include "snd.h"
#include "dbs.h"
#include "pair.h"

#include "CVideoDriver.h"
#include "CSoundAnalyzer.h"
#include "CSettingsManager.h"
#include "CSongDataManager.h"
#include "visualizers/CRoundVisualizer.h"
#include "visualizers/CStraightVisualizer.h"

#define NUM_COLORS 36
#define TRACK_TITLE_LENGTH_MAX 128

#define BUTTON_HOLD_MILLIS 500
#define BUTTON_CONT_MILLIS 200
#define SHUTDOWN_MILLIS 4000

#define DEFAULT_VISUALIZER_INDEX 0
#define DEFAULT_VISUALIZER_STYLE 0
#define DEFAULT_VISUALIZER_COLORP_R 22
#define DEFAULT_VISUALIZER_COLORP_G 22
#define DEFAULT_VISUALIZER_COLORP_B 229
#define DEFAULT_VISUALIZER_COLORS_R 229
#define DEFAULT_VISUALIZER_COLORS_G 22
#define DEFAULT_VISUALIZER_COLORS_B 22
#define DEFAULT_VISUALIZER_RANDOMIZER 0

#define DEFAULT_VIDEO_SIZE_X 32
#define DEFAULT_VIDEO_SIZE_Y 16
#define DEFAULT_VIDEO_OVERSAMPLE 1
#define DEFAULT_VIDEO_ROTATION 0

//constants
static const char* settingsFile = "data/settings.txt";
static const char* songdataFile = "data/songdata.txt";

//data
CVideoDriver* videoDriver=0;
CSoundAnalyzer* soundAnalyzer=0;
CSettingsManager* settingsManager=0;
CSongDataManager* songDataManager=0;
struct LEDPanelOptions ledPanelOptions;
struct LEDStripOptions ledStripOptions;
clock_t buttonTimeStamps[INP_BTN_TOTAL];
int buttonTicks[INP_BTN_TOTAL];
Color colorPalettePrimary[NUM_COLORS];
Color colorPaletteSecondary[NUM_COLORS];
char trackTitle[TRACK_TITLE_LENGTH_MAX];
char trackAlbum[TRACK_TITLE_LENGTH_MAX];
char trackArtist[TRACK_TITLE_LENGTH_MAX];
int defaultVisualizer;
int defaultStyle;
Color defaultPrimaryColor;
Color defaultSecondaryColor;
bool defaultRandomizer;
	
//functions
void core_initSettings();
void core_initColors();
void core_initTrackData();
void core_initButtons();
void core_loadVideoSettings(struct LEDPanelOptions** panel, struct LEDStripOptions** strip, Vector* size, int* oversample, int* rotation);
bool core_checkTrackData();
bool core_checkButtonState(char button, clock_t now);
void core_updateVolume(int change);
void core_updateBrightness(int change);
void core_drawEditMode();
void core_close();

//program entry
int main(int argc, char** argv)
{
	//load core settings
	settingsManager = new CSettingsManager(settingsFile);
	struct LEDPanelOptions* panelOptions = 0;
	struct LEDStripOptions* stripOptions = 0;
	Vector size = Vector(0,0);
	int oversample = 0;
	int rotation = 0;
	core_loadVideoSettings(&panelOptions, &stripOptions, &size, &oversample, &rotation);
	
	//init core modules
	if(led_init(panelOptions, stripOptions) || inp_init() || bt_init() || snd_init(0) || dbs_init() || pair_init()) {
		printf("Init failed. Are you running as root??\n");
		core_close();
		return 1;
	}
	
	//init high level managers
	videoDriver = new CVideoDriver(size, oversample, rotation);
	soundAnalyzer = new CSoundAnalyzer();
	songDataManager = new CSongDataManager(songdataFile);
	
	//init data
	char editMode = 0;
	int visualizer = 0;
	int color = 0;
	core_initColors();
	core_initTrackData();
	core_initButtons();
	core_initSettings();
	
	//set manager parameters
	songDataManager->enableDefaulter(defaultRandomizer);
	
	//init visualizers
	const int numVisualizers = 4;
	IVisualizer* visualizers[numVisualizers];
	visualizers[0] = new CRoundVisualizer(videoDriver, soundAnalyzer, 0);
	visualizers[1] = new CRoundVisualizer(videoDriver, soundAnalyzer, 90);
	visualizers[2] = new CStraightVisualizer(videoDriver, soundAnalyzer, 0);
	visualizers[3] = new CStraightVisualizer(videoDriver, soundAnalyzer, 90);
	
	//set default visualizer settings
	visualizer = (defaultVisualizer < numVisualizers) ? defaultVisualizer : 0;
	visualizers[visualizer]->setup();
	visualizers[visualizer]->setStyle(defaultStyle);
	visualizers[visualizer]->setColors(defaultPrimaryColor, defaultSecondaryColor);
	
	//loop
	clock_t lastTick = clock();
	while(true) {
		clock_t now = clock();
		double elapsedTime = (double)(now - lastTick)/CLOCKS_PER_SEC;
		lastTick = now;
		//printf("Time elpased is %f seconds\n", elapsedTime);/////////////////////////////////////////////////////////////////////////////
		
		//check track change
		if(core_checkTrackData()) {
			if(songDataManager->findSongData(pair_mediaGetTrackArtist(), pair_mediaGetTrackAlbum(), pair_mediaGetTrackTitle())) {
				int songVisualizer = songDataManager->getVisualizer() % numVisualizers;
				if(songVisualizer != visualizer) {
					visualizers[visualizer]->clear();
					visualizers[songVisualizer]->setup();
					visualizer = songVisualizer;
				}
				visualizers[visualizer]->setStyle(songDataManager->getStyle());
				visualizers[visualizer]->setColors(songDataManager->getColorPrimary(), songDataManager->getColorSecondary());
			} else {
				//no saved settings, use defaults
				int songVisualizer = defaultVisualizer % numVisualizers;
				if(songVisualizer != visualizer) {
					visualizers[visualizer]->clear();
					visualizers[songVisualizer]->setup();
					visualizer = songVisualizer;
				}
				visualizers[visualizer]->setStyle(defaultStyle);
				visualizers[visualizer]->setColors(defaultPrimaryColor, defaultSecondaryColor);
			}
		}
		
		//input
		inp_updateButtonState();
		core_checkButtonState(INP_BTN_PWR, now);
		if((BUTTON_HOLD_MILLIS+buttonTicks[INP_BTN_PWR]*BUTTON_CONT_MILLIS) > SHUTDOWN_MILLIS) system("sudo shutdown now");
		if(core_checkButtonState(INP_BTN_VOLUP, now)) core_updateVolume(5);
		if(core_checkButtonState(INP_BTN_VOLDOWN, now)) core_updateVolume(-5);
		if(editMode==0) {
			if(inp_getButtonState(INP_BTN_MENU)==1) editMode=1;
			if(core_checkButtonState(INP_BTN_UP, now)) core_updateBrightness(5);
			if(core_checkButtonState(INP_BTN_DOWN, now)) core_updateBrightness(-5);
			if(inp_getButtonState(INP_BTN_LEFT)==1) pair_mediaPrevious();
			if(inp_getButtonState(INP_BTN_RIGHT)==1) pair_mediaNext();
			if(inp_getButtonState(INP_BTN_OK)==1 && pair_mediaGetState()!=PAIR_MEDIA_STATE_NOT_CONNECTED) {
				if(pair_mediaGetState()==PAIR_MEDIA_STATE_PLAYING) pair_mediaPause();
				else pair_mediaPlay();
			}
			
		} else {
			int oldVisualizer = visualizer;
			int oldColor = color;
			if(inp_getButtonState(INP_BTN_MENU)==1) {
				editMode=0;
				if(pair_mediaGetTrackTitle() || pair_mediaGetTrackAlbum() || pair_mediaGetTrackArtist()) {
					//save settings for track
					songDataManager->saveSongData(pair_mediaGetTrackArtist(), pair_mediaGetTrackAlbum(), pair_mediaGetTrackTitle(), 
						visualizer, visualizers[visualizer]->getStyle(), colorPalettePrimary[color], colorPaletteSecondary[color]);
				}
			}
			if(core_checkButtonState(INP_BTN_OK, now)) visualizer++;
			if(core_checkButtonState(INP_BTN_BACK, now)) visualizer--;
			if(core_checkButtonState(INP_BTN_UP, now)) visualizers[visualizer]->setStyle(visualizers[visualizer]->getStyle()+1);
			if(core_checkButtonState(INP_BTN_DOWN, now)) visualizers[visualizer]->setStyle(visualizers[visualizer]->getStyle()-1);
			if(core_checkButtonState(INP_BTN_LEFT, now)) color--;
			if(core_checkButtonState(INP_BTN_RIGHT, now)) color++;
			if(visualizer < 0) visualizer = numVisualizers-1;
			if(visualizer >= numVisualizers) visualizer = 0;
			if(visualizer != oldVisualizer) {
				visualizers[oldVisualizer]->clear();
				visualizers[visualizer]->setup();
				visualizers[visualizer]->setStyle(0);
				visualizers[visualizer]->setColors(colorPalettePrimary[color], colorPaletteSecondary[color]);
			}
			if(color < 0) color = NUM_COLORS-1;
			if(color >= NUM_COLORS) color = 0;
			if(color != oldColor) {
				visualizers[visualizer]->setColors(colorPalettePrimary[color], colorPaletteSecondary[color]);
			}			
		}
		
		//render
		soundAnalyzer->refresh();
		visualizers[visualizer]->draw(elapsedTime);
		if(editMode==1) core_drawEditMode();
		
		//flush to display
		videoDriver->flush();
	}
	
	//cleanup
	for(int i=0; i<numVisualizers; i++) delete visualizers[i];	
	delete soundAnalyzer;
	delete videoDriver;
	delete songDataManager;
	delete settingsManager;

	//exit code
	core_close();
	return 0;
}

//functions
void core_initSettings() {
	snd_setVolume(settingsManager->getPropertyInteger("system.volume", 80));
	led_setBrightness(settingsManager->getPropertyInteger("system.brightness", 80));
	
	defaultVisualizer = settingsManager->getPropertyInteger("visualizer.default.index", DEFAULT_VISUALIZER_INDEX);
	defaultStyle = settingsManager->getPropertyInteger("visualizer.default.style", DEFAULT_VISUALIZER_STYLE);
	defaultPrimaryColor = Color(settingsManager->getPropertyInteger("visualizer.default.colorp.red", DEFAULT_VISUALIZER_COLORP_R), 
		settingsManager->getPropertyInteger("visualizer.default.colorp.green", DEFAULT_VISUALIZER_COLORP_G), 
		settingsManager->getPropertyInteger("visualizer.default.colorp.blue", DEFAULT_VISUALIZER_COLORP_B));
	defaultSecondaryColor = Color(settingsManager->getPropertyInteger("visualizer.default.colors.red", DEFAULT_VISUALIZER_COLORS_R), 
		settingsManager->getPropertyInteger("visualizer.default.colors.green", DEFAULT_VISUALIZER_COLORS_G), 
		settingsManager->getPropertyInteger("visualizer.default.colors.blue", DEFAULT_VISUALIZER_COLORS_B));
	defaultRandomizer = settingsManager->getPropertyInteger("visualizer.default.randomizer", DEFAULT_VISUALIZER_RANDOMIZER) > 0;
	
	int code;
	code = settingsManager->getPropertyInteger("input.code.up", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_UP, code);
	code = settingsManager->getPropertyInteger("input.code.down", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_DOWN, code);
	code = settingsManager->getPropertyInteger("input.code.left", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_LEFT, code);
	code = settingsManager->getPropertyInteger("input.code.right", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_RIGHT, code);
	code = settingsManager->getPropertyInteger("input.code.home", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_HOME, code);
	code = settingsManager->getPropertyInteger("input.code.menu", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_MENU, code);
	code = settingsManager->getPropertyInteger("input.code.back", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_BACK, code);
	code = settingsManager->getPropertyInteger("input.code.ok", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_OK, code);
	code = settingsManager->getPropertyInteger("input.code.volup", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_VOLUP, code);
	code = settingsManager->getPropertyInteger("input.code.voldown", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_VOLDOWN, code);
	code = settingsManager->getPropertyInteger("input.code.pwr", -1);
	if(code > -1) inp_setButtonCode(INP_BTN_PWR, code);
}
void core_initColors() {
	for(int i=0; i<NUM_COLORS/4; i++) {
		colorPalettePrimary[i+(NUM_COLORS/4)*0] = Color::fromHSV((i*(360/(NUM_COLORS/4)))%360,90,90);
		colorPaletteSecondary[i+(NUM_COLORS/4)*0] = Color::fromHSV(((i*(360/(NUM_COLORS/4))+40))%360,90,90);
		colorPalettePrimary[i+(NUM_COLORS/4)*1] = Color::fromHSV((i*(360/(NUM_COLORS/4)))%360,90,90);
		colorPaletteSecondary[i+(NUM_COLORS/4)*1] = Color::fromHSV(((i*(360/(NUM_COLORS/4))+80))%360,90,90);
		colorPalettePrimary[i+(NUM_COLORS/4)*2] = Color::fromHSV((i*(360/(NUM_COLORS/4)))%360,90,90);
		colorPaletteSecondary[i+(NUM_COLORS/4)*2] = Color::fromHSV(((i*(360/(NUM_COLORS/4))+120))%360,90,90);
		colorPalettePrimary[i+(NUM_COLORS/4)*3] = Color::fromHSV((i*(360/(NUM_COLORS/4)))%360,90,90);
		colorPaletteSecondary[i+(NUM_COLORS/4)*3] = Color(0,0,0);
	}
}
void core_initTrackData() {
	trackTitle[0] = 0;
	trackAlbum[0] = 0;
	trackArtist[0] = 0;
	trackTitle[TRACK_TITLE_LENGTH_MAX-1] = 0;
	trackAlbum[TRACK_TITLE_LENGTH_MAX-1] = 0;
	trackArtist[TRACK_TITLE_LENGTH_MAX-1] = 0;
}
void core_initButtons() {
	clock_t now = clock();
	for(int i=0; i<INP_BTN_TOTAL; i++) {
		buttonTimeStamps[i] = now;
		buttonTicks[i] = 0;
	}
}
void core_loadVideoSettings(struct LEDPanelOptions** panel, struct LEDStripOptions** strip, Vector* size, int* oversample, int* rotation) {
	//genreal video settings
	size->X = settingsManager->getPropertyInteger("video.system.size_x", DEFAULT_VIDEO_SIZE_X);
	size->Y = settingsManager->getPropertyInteger("video.system.size_y", DEFAULT_VIDEO_SIZE_Y);
	*oversample = settingsManager->getPropertyInteger("video.system.oversample", DEFAULT_VIDEO_OVERSAMPLE);
	*rotation = settingsManager->getPropertyInteger("video.system.rotation", DEFAULT_VIDEO_ROTATION);
	
	char videoDriver[32];
	settingsManager->getPropertyString("video.system.driver", "led_panel", videoDriver, 32);
	if(strcmp("led_panel", videoDriver)==0) {
		
		//led panel driver
		struct LEDPanelOptions panelDefaults;
		led_setPanelDefaults(&panelDefaults);
		settingsManager->getPropertyString("video.led_panel.led_gpio_mapping", panelDefaults.hardware_mapping, ledPanelOptions.hardware_mapping, 32);
		ledPanelOptions.rows = settingsManager->getPropertyInteger("video.led_panel.led_rows", panelDefaults.rows);
		ledPanelOptions.cols = settingsManager->getPropertyInteger("video.led_panel.led_cols", panelDefaults.cols);
		ledPanelOptions.chain_length = settingsManager->getPropertyInteger("video.led_panel.led_chain", panelDefaults.chain_length);
		ledPanelOptions.parallel = settingsManager->getPropertyInteger("video.led_panel.led_parallel", panelDefaults.parallel);
		ledPanelOptions.pwm_bits = settingsManager->getPropertyInteger("video.led_panel.led_pwm_bits", panelDefaults.pwm_bits);
		ledPanelOptions.pwm_lsb_nanoseconds = settingsManager->getPropertyInteger("video.led_panel.led_pwm_lsb_nanoseconds", panelDefaults.pwm_lsb_nanoseconds);
		ledPanelOptions.pwm_dither_bits = settingsManager->getPropertyInteger("video.led_panel.led_pwm_dither_bits", panelDefaults.pwm_dither_bits);
		ledPanelOptions.scan_mode = settingsManager->getPropertyInteger("video.led_panel.led_scan_mode", panelDefaults.scan_mode);
		ledPanelOptions.row_address_type = settingsManager->getPropertyInteger("video.led_panel.led_row_addr_type", panelDefaults.row_address_type);
		ledPanelOptions.multiplexing = settingsManager->getPropertyInteger("video.led_panel.led_multiplexing", panelDefaults.multiplexing);
		settingsManager->getPropertyString("video.led_panel.led_rgb_sequence", panelDefaults.led_rgb_sequence, ledPanelOptions.led_rgb_sequence, 8);
		settingsManager->getPropertyString("video.led_panel.led_pixel_mapper", panelDefaults.pixel_mapper_config, ledPanelOptions.pixel_mapper_config, 128);
		settingsManager->getPropertyString("video.led_panel.led_panel_type", panelDefaults.panel_type, ledPanelOptions.panel_type, 16);
		ledPanelOptions.disable_hardware_pulsing = settingsManager->getPropertyInteger("video.led_panel.led_no_hardware_pulse", panelDefaults.disable_hardware_pulsing);
		ledPanelOptions.inverse_colors = settingsManager->getPropertyInteger("video.led_panel.led_inverse", panelDefaults.inverse_colors);
		ledPanelOptions.mirror_x = settingsManager->getPropertyInteger("video.led_panel.led_mirror_x", panelDefaults.mirror_x);
		ledPanelOptions.mirror_y = settingsManager->getPropertyInteger("video.led_panel.led_mirror_y", panelDefaults.mirror_y);
		*panel = &ledPanelOptions;
		*strip = 0;
	} else {
		
		//led strip driver
		struct LEDStripOptions stripDefaults;
		led_setStripDefaults(&stripDefaults);
		ledStripOptions.dimension_x = settingsManager->getPropertyInteger("video.led_strip.dimension_x", stripDefaults.dimension_x);
		ledStripOptions.dimension_y = settingsManager->getPropertyInteger("video.led_strip.dimension_y", stripDefaults.dimension_y);
		ledStripOptions.skip = settingsManager->getPropertyInteger("video.led_strip.skip", stripDefaults.skip);
		settingsManager->getPropertyString("video.led_strip.layout", stripDefaults.layout, ledStripOptions.layout, 4);
		settingsManager->getPropertyString("video.led_strip.rgb_sequence", stripDefaults.rgb_sequence, ledStripOptions.rgb_sequence, 8);
		ledStripOptions.frequency = settingsManager->getPropertyInteger("video.led_strip.frequency", stripDefaults.frequency);
		ledStripOptions.invert = settingsManager->getPropertyInteger("video.led_strip.invert", stripDefaults.invert);
		ledStripOptions.mirror_x = settingsManager->getPropertyInteger("video.led_strip.mirror_x", stripDefaults.mirror_x);
		ledStripOptions.mirror_y = settingsManager->getPropertyInteger("video.led_strip.mirror_y", stripDefaults.mirror_y);
		*strip = &ledStripOptions;
		*panel = 0;
	}
}
bool core_checkTrackData() {
	bool changed = false;
	if(strcmp(trackTitle, pair_mediaGetTrackTitle())!=0) {
		strncpy(trackTitle, pair_mediaGetTrackTitle(), TRACK_TITLE_LENGTH_MAX-1);
		changed = true;
	}
	if(strcmp(trackAlbum, pair_mediaGetTrackAlbum())!=0) {
		strncpy(trackAlbum, pair_mediaGetTrackAlbum(), TRACK_TITLE_LENGTH_MAX-1);
		changed = true;
	}
	if(strcmp(trackArtist, pair_mediaGetTrackArtist())!=0) {
		strncpy(trackArtist, pair_mediaGetTrackArtist(), TRACK_TITLE_LENGTH_MAX-1);
		changed = true;
	}	
	return changed;
}
bool core_checkButtonState(char button, clock_t now) {
	if(inp_getButtonState(button) == 1) {
		buttonTimeStamps[button] = now;
		buttonTicks[button] = 0;
		return true;
	} else if(inp_getButtonState(button) > 1) {
		int millis = ((int)(now - buttonTimeStamps[button])*1000)/CLOCKS_PER_SEC;
		if(millis > BUTTON_HOLD_MILLIS + buttonTicks[button]*BUTTON_CONT_MILLIS) {
			buttonTicks[button]++;
			return true;
		}
	}
	return false;
}
void core_updateVolume(int change) {
	int oldVal = snd_getVolume();
	int newVal = oldVal + change;
	if(newVal < 0) newVal = 0;
	if(newVal > 100) newVal = 100;
	snd_setVolume(newVal);
	if(oldVal != newVal) {
		settingsManager->setPropertyInteger("system.volume", newVal);
	}
}
void core_updateBrightness(int change) {
	int oldVal = led_getBrightness();
	int newVal = oldVal + change;
	if(newVal < 1) newVal = 1;
	if(newVal > 100) newVal = 100;
	led_setBrightness(newVal);
	if(oldVal != newVal) {
		settingsManager->setPropertyInteger("system.brightness", newVal);
	}	
}
void core_drawEditMode() {
	int videoOversample = videoDriver->getOversample();
	int videoDimX = videoDriver->getDimension().X;
	int videoDimY = videoDriver->getDimension().Y;
	int size = videoDimY/16;
	if(size > videoDimX/16) size = videoDimX/16;
	if(size < videoOversample) size = videoOversample;
	videoDriver->drawTri(Vector(size,0), Color(255,255,255), Vector(0,0), Color(255,255,255), Vector(0,size), Color(255,255,255));
	videoDriver->drawTri(Vector((videoDimX-1)-size,0), Color(255,255,255), Vector((videoDimX-1),0), Color(255,255,255), Vector((videoDimX-1),size), Color(255,255,255));
	videoDriver->drawTri(Vector(size,(videoDimY-1)), Color(255,255,255), Vector(0,(videoDimY-1)), Color(255,255,255), Vector(0,(videoDimY-1)-size), Color(255,255,255));
	videoDriver->drawTri(Vector((videoDimX-1)-size,(videoDimY-1)), Color(255,255,255), Vector((videoDimX-1),(videoDimY-1)), Color(255,255,255), Vector((videoDimX-1),(videoDimY-1)-size), Color(255,255,255));
}
void core_close() {
	pair_close();
	dbs_close();
	snd_close();
	bt_close();
	inp_close();
	led_close();
}
