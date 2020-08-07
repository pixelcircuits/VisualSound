//-----------------------------------------------------------------------------------------
// Title:	Music Visualizer Template
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CTemplateVisualizer.h" /*** rename according to your visualizer ***/
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
/*** remember to add any additional needed includes here ***/

/*** define the different style modes yor visualizer can have ***/
#define STYLE_ONE 0
#define STYLE_TWO 1
#define STYLE_THREE 2
#define NUM_STYLES 3

//! Main Constructor
CTemplateVisualizer::CTemplateVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0)
{
	/*** here is where you should place initialization   ***
	 *** setup that will last the whole program duration ***/
}

//! Destructor
CTemplateVisualizer::~CTemplateVisualizer()
{
	/*** here is where you should place the cleanup ***
	 *** process at the end of program execution    ***/
	clear();
}

//! Sets up the visualizer
void CTemplateVisualizer::setup()
{
	/*** this method is called when switching to this visualizer ***/
	
	/*** one thing you should make sure to do is set sound analyzer     ***
	 *** properties according to how you want your visualizer to behave ***/
	videoDriver->setRotation(0);
	soundAnalyzer->setSamplingFrequency(SND_DEFAULT_SAMPLE_FREQUENCY);
	soundAnalyzer->setWaveLPF(SND_DEFAULT_WAVE_LP_FILTER);
	soundAnalyzer->setWaveTimeSmooth(SND_DEFAULT_WAVE_TIME_SMOOTH);
	soundAnalyzer->setSpecSmoothPass(SND_DEFAULT_SPEC_SMOOTH_PASS);
	soundAnalyzer->setSpecTimeSmooth(SND_DEFAULT_SPEC_TIME_SMOOTH);
}

//! Draws the visualizer
void CTemplateVisualizer::draw(double elapsedTime)
{
	/*** this method draws the visualizer ***/
	
	/*** It's recomended to do your sound anlysis variable computation first ***/
	//float intensity = (float)(soundAnalyzer->getVURight()+soundAnalyzer->getVULeft())/6000.0f;
	//float trebIntensity = (float)(soundAnalyzer->getTrebRight()+soundAnalyzer->getTrebLeft())/24000.0f;
	//float midIntensity = (float)(soundAnalyzer->getMidRight()+soundAnalyzer->getMidLeft())/24000.0f;
	//float bassIntensity = (float)(soundAnalyzer->getBassRight()+soundAnalyzer->getBassLeft())/24000.0f;
	//int videoOversample = videoDriver->getOversample();
	//int videoDimX = videoDriver->getDimension().X;
	//int videoDimY = videoDriver->getDimension().Y;
	
	/*** you'll probably want to start by clearing out the display ***/
	videoDriver->clearVideoBuffer(Color(0,0,0));
	
	/*** now draw the visualizer and remember   ***
	 *** to use the set colors (color1, color2) ***/
}

//! Sets the visualizer colors
void CTemplateVisualizer::setColors(Color color1, Color color2)
{
	/*** this should be left alone to allow colors to be configured ***/
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CTemplateVisualizer::getColor(int num)
{
	/*** this should be left alone to allow colors to be configured ***/
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CTemplateVisualizer::setStyle(int style)
{
	/*** this should be left alone to allow style to be configured ***/
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	this->style = style;
}

//! Gets the visualizer style
int CTemplateVisualizer::getStyle()
{
	/*** this should be left alone to allow style to be configured ***/
	return style;
}

//! Clears all resources of the visualizer
void CTemplateVisualizer::clear()
{
	/*** this method is called when switching to another visualizer ***/
	/*** this is where you should do any cleanup ***
	 *** before the next visualizer starts       ***/
}
