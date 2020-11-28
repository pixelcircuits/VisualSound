//-----------------------------------------------------------------------------------------
// Title:   Music and Sound Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
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
#include "visualizers/CGeometryVisualizer.h"
#include "visualizers/CWaveVisualizer.h"
#include "visualizers/CParticleVisualizer.h"

#define NUM_COLORS 36
#define TRACK_TITLE_LENGTH_MAX 128

#define BUTTON_HOLD_MILLIS 500
#define BUTTON_CONT_MILLIS 200
#define SHUTDOWN_MILLIS 2000

#define DEFAULT_VISUALIZER_INDEX 0
#define DEFAULT_VISUALIZER_STYLE 0
#define DEFAULT_VISUALIZER_COLORP_R 22
#define DEFAULT_VISUALIZER_COLORP_G 22
#define DEFAULT_VISUALIZER_COLORP_B 229
#define DEFAULT_VISUALIZER_COLORS_R 229
#define DEFAULT_VISUALIZER_COLORS_G 22
#define DEFAULT_VISUALIZER_COLORS_B 22
#define DEFAULT_VISUALIZER_RANDOMIZER 0
#define DEFAULT_VISUALIZER_RANDOMIZER_TIME 0

#define DEFAULT_VIDEO_SIZE_X 10
#define DEFAULT_VIDEO_SIZE_Y 10
#define DEFAULT_VIDEO_SIZE_Z 10
#define DEFAULT_VIDEO_OVERSAMPLE 1

//constants
static const char* settingsFile = "data/settings.txt";
static const char* songdataFile = "data/songdata.txt";

//data
CVideoDriver* videoDriver=0;
CSoundAnalyzer* soundAnalyzer=0;
CSettingsManager* settingsManager=0;
CSongDataManager* songDataManager=0;
double lastGetTime;
double buttonTimeStamps[INP_BTN_TOTAL];
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
int defaultRandomizerTime;
double lastRandomizer;
	
//functions
void core_initSettings();
void core_initColors();
void core_initTrackData();
void core_initButtons();
void core_loadVideoSettings(Vector* size, int* oversample);
bool core_checkTrackData();
bool core_checkButtonState(char button, double now);
void core_updateVolume(int change);
void core_updateBrightness(int change);
void core_drawEditMode();
double core_getTime();
void core_close();

//program entry
int main(int argc, char** argv)
{
	//load core settings
	settingsManager = new CSettingsManager(settingsFile);
	Vector size = Vector(0,0,0);
	int oversample = 0;
	core_loadVideoSettings(&size, &oversample);
	
	//init core modules
	if(led_init() || inp_init() || bt_init() || snd_init(0) || dbs_init() || pair_init()) {
		printf("Init failed. Are you running as root??\n");
		core_close();
		return 1;
	}
	
	//init high level managers
	videoDriver = new CVideoDriver(size, oversample);
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
	lastRandomizer = 0;
	
	//set manager parameters
	songDataManager->enableDefaulter(defaultRandomizer);
	
	//init visualizers
	const int numVisualizers = 3;
	IVisualizer* visualizers[numVisualizers];
	visualizers[0] = new CGeometryVisualizer(videoDriver, soundAnalyzer);
	visualizers[1] = new CWaveVisualizer(videoDriver, soundAnalyzer);
	visualizers[2] = new CParticleVisualizer(videoDriver, soundAnalyzer);
	
	//set default visualizer settings
	visualizer = (defaultVisualizer < numVisualizers) ? defaultVisualizer : 0;
	visualizers[visualizer]->setup();
	visualizers[visualizer]->setStyle(defaultStyle);
	visualizers[visualizer]->setColors(defaultPrimaryColor, defaultSecondaryColor);
	
	//loop
	double lastTick = core_getTime();
	while(true) {
		double now = core_getTime();
		
		//check track change
		if(defaultRandomizerTime == 0) {
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
			
		} else {
			//check for randomizing visualizer
			if((now - lastRandomizer) > defaultRandomizerTime) {
				lastRandomizer = now;
				
				int randColor = rand()%NUM_COLORS;
				int songVisualizer = rand()%numVisualizers;
				if(songVisualizer != visualizer) {
					visualizers[visualizer]->clear();
					visualizers[songVisualizer]->setup();
					visualizer = songVisualizer;
				}
				visualizers[visualizer]->setStyle(rand()%100);
				visualizers[visualizer]->setColors(colorPalettePrimary[randColor], colorPaletteSecondary[randColor]);
			}
		}
		
		//input
		inp_updateButtonState();
		core_checkButtonState(INP_BTN_PWR, now);
		if((BUTTON_HOLD_MILLIS+buttonTicks[INP_BTN_PWR]*BUTTON_CONT_MILLIS) > SHUTDOWN_MILLIS) {
			led_standby();
			system("sudo shutdown now");
		}
		core_checkButtonState(INP_BTN_HOME, now);
		if((BUTTON_HOLD_MILLIS+buttonTicks[INP_BTN_HOME]*BUTTON_CONT_MILLIS) > SHUTDOWN_MILLIS) break;
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
			lastRandomizer = now;
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
		
		//run sound analysis
		soundAnalyzer->refresh();
		
		//update time
		double time = core_getTime();
		double elapsedTime = time - lastTick;
		lastTick = time;
		//printf("Time elpased is %f seconds\n", elapsedTime);/////////////////////////////////////////////////////////////////////////////
		
		//render
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
	inp_setButtonHold(INP_BTN_PWR, (~settingsManager->getPropertyInteger("system.poweroff.hold", 1)) & 0x01);
	
	defaultVisualizer = settingsManager->getPropertyInteger("visualizer.default.index", DEFAULT_VISUALIZER_INDEX);
	defaultStyle = settingsManager->getPropertyInteger("visualizer.default.style", DEFAULT_VISUALIZER_STYLE);
	defaultPrimaryColor = Color(settingsManager->getPropertyInteger("visualizer.default.colorp.red", DEFAULT_VISUALIZER_COLORP_R), 
		settingsManager->getPropertyInteger("visualizer.default.colorp.green", DEFAULT_VISUALIZER_COLORP_G), 
		settingsManager->getPropertyInteger("visualizer.default.colorp.blue", DEFAULT_VISUALIZER_COLORP_B));
	defaultSecondaryColor = Color(settingsManager->getPropertyInteger("visualizer.default.colors.red", DEFAULT_VISUALIZER_COLORS_R), 
		settingsManager->getPropertyInteger("visualizer.default.colors.green", DEFAULT_VISUALIZER_COLORS_G), 
		settingsManager->getPropertyInteger("visualizer.default.colors.blue", DEFAULT_VISUALIZER_COLORS_B));
	defaultRandomizer = settingsManager->getPropertyInteger("visualizer.default.randomizer", DEFAULT_VISUALIZER_RANDOMIZER) > 0;
	defaultRandomizerTime = settingsManager->getPropertyInteger("visualizer.default.randomizer.time", DEFAULT_VISUALIZER_RANDOMIZER_TIME);
	
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
	double now = core_getTime();
	for(int i=0; i<INP_BTN_TOTAL; i++) {
		buttonTimeStamps[i] = now;
		buttonTicks[i] = 0;
	}
}
void core_loadVideoSettings(Vector* size, int* oversample) {
	//genreal video settings
	size->X = settingsManager->getPropertyInteger("video.system.size_x", DEFAULT_VIDEO_SIZE_X);
	size->Y = settingsManager->getPropertyInteger("video.system.size_y", DEFAULT_VIDEO_SIZE_Y);
	size->Z = settingsManager->getPropertyInteger("video.system.size_z", DEFAULT_VIDEO_SIZE_Z);
	*oversample = settingsManager->getPropertyInteger("video.system.oversample", DEFAULT_VIDEO_OVERSAMPLE);
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
bool core_checkButtonState(char button, double now) {
	if(inp_getButtonState(button) == 1) {
		buttonTimeStamps[button] = now;
		buttonTicks[button] = 0;
		return true;
	} else if(inp_getButtonState(button) > 1) {
		int millis = (int)((now - buttonTimeStamps[button])*1000.0);
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
	int xmax = videoDriver->getDimension().X-(videoOversample);
	int ymax = videoDriver->getDimension().Y-(videoOversample);
	int zmax = videoDriver->getDimension().Z-(videoOversample);
	int size = 1*videoOversample;
	
	Color color = Color(200,200,200);
	for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
		videoDriver->drawLine(Vector(size,0,0)+Vector(x,y,z), color, Vector(0,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,size,0)+Vector(x,y,z), color, Vector(0,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,0,size)+Vector(x,y,z), color, Vector(0,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax-size,0,0)+Vector(x,y,z), color, Vector(xmax,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,size,0)+Vector(x,y,z), color, Vector(xmax,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,0,size)+Vector(x,y,z), color, Vector(xmax,0,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax-size,0,zmax)+Vector(x,y,z), color, Vector(xmax,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,size,zmax)+Vector(x,y,z), color, Vector(xmax,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,0,zmax-size)+Vector(x,y,z), color, Vector(xmax,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(size,0,zmax)+Vector(x,y,z), color, Vector(0,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,size,zmax)+Vector(x,y,z), color, Vector(0,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,0,zmax-size)+Vector(x,y,z), color, Vector(0,0,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(size,ymax,0)+Vector(x,y,z), color, Vector(0,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,ymax-size,0)+Vector(x,y,z), color, Vector(0,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,ymax,size)+Vector(x,y,z), color, Vector(0,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax-size,ymax,0)+Vector(x,y,z), color, Vector(xmax,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,ymax-size,0)+Vector(x,y,z), color, Vector(xmax,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,ymax,size)+Vector(x,y,z), color, Vector(xmax,ymax,0)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax-size,ymax,zmax)+Vector(x,y,z), color, Vector(xmax,ymax,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,ymax-size,zmax)+Vector(x,y,z), color, Vector(xmax,ymax,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(xmax,ymax,zmax-size)+Vector(x,y,z), color, Vector(xmax,ymax,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(size,ymax,zmax)+Vector(x,y,z), color, Vector(0,ymax,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,ymax-size,zmax)+Vector(x,y,z), color, Vector(0,ymax,zmax)+Vector(x,y,z), color);
		videoDriver->drawLine(Vector(0,ymax,zmax-size)+Vector(x,y,z), color, Vector(0,ymax,zmax)+Vector(x,y,z), color);
	}
}
double core_getTime() {
    struct timeval tv;
    if(gettimeofday(&tv,NULL) > -1) {
		lastGetTime = (double)tv.tv_sec + ((double)tv.tv_usec)/(1000.0*1000.0);
	}
	return lastGetTime;
}
void core_close() {
	pair_close();
	dbs_close();
	snd_close();
	bt_close();
	inp_close();
	led_close();
}
