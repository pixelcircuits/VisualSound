//-----------------------------------------------------------------------------------------
// Title:	Straight Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CStraightVisualizer.h"
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
#include <math.h>
#include <stdio.h>

#define STYLE_FULL 0
#define STYLE_NO_WAVE 1
#define STYLE_NO_SPEC 2
#define NUM_STYLES 3

//! Main Constructor
CStraightVisualizer::CStraightVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa, int r) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0), rotation(r)
{
}

//! Destructor
CStraightVisualizer::~CStraightVisualizer()
{
	clear();
}

//! Sets up the visualizer
void CStraightVisualizer::setup()
{
	videoDriver->setRotation(rotation);
	soundAnalyzer->setSamplingFrequency(SND_DEFAULT_SAMPLE_FREQUENCY);
	soundAnalyzer->setWaveLPF(0.3f);
	soundAnalyzer->setWaveTimeSmooth(0.6f);
	soundAnalyzer->setSpecSmoothPass(8);
	soundAnalyzer->setSpecTimeSmooth(0.3f);
}

//! Draws the visualizer
void CStraightVisualizer::draw(double elapsedTime)
{
	//compute basic values
	float intensity = (float)(soundAnalyzer->getVURight()+soundAnalyzer->getVULeft())/6000.0f;
	float bassIntensity = (float)(soundAnalyzer->getBassRight()+soundAnalyzer->getBassLeft())/24000.0f;
	float cappedIntensity = (intensity > 1.0f) ? 1.0f : intensity;
	int videoOversample = videoDriver->getOversample();
	int videoDimX = videoDriver->getDimension().X;
	int videoDimY = videoDriver->getDimension().Y;
	
	//clear buffer
	videoDriver->clearVideoBuffer(Color(0,0,0));
	
	//edge gradients
	Color colorG = color1; 
	colorG.lerp(color2, 50);
	colorG.lerp(Color(0,0,0), 50 - (int)(25.0f*cappedIntensity));
	int gradientSize = videoDimY/5 + (int)((float)(videoDimY/10)*cappedIntensity);
	videoDriver->drawQuad(Vector(0, 0), colorG, 
		Vector(videoDimX-1, 0), colorG, 
		Vector(videoDimX-1, gradientSize), Color(0,0,0),
		Vector(0, gradientSize), Color(0,0,0));
	videoDriver->drawQuad(Vector(0, videoDimY - gradientSize), Color(0,0,0), 
		Vector(videoDimX-1, videoDimY - gradientSize), Color(0,0,0), 
		Vector(videoDimX-1, videoDimY-1), colorG,
		Vector(0, videoDimY-1), colorG);
		
	//waves with black outlines
	if(style==STYLE_FULL || style==STYLE_NO_SPEC) {
		int waveOffset = videoDimY/6;
		int waveStart = 20;
		float waveLength = 0.4f*(((float)SND_BUFFER_SAMPLE_SIZE/(float)videoDimX)/1);
		float waveAmplitude = 0.02f*((float)videoDimY/1000.0f);
		if(style==STYLE_NO_SPEC) {
			waveOffset = videoDimY/3;
		}
		if(color2.Red==0 && color2.Green==0 && color2.Blue==0) {
			waveOffset = videoDimY/2;
			waveAmplitude *= 1.5f;
		}
		
		//left wave
		for(int i=0; i<videoDimX; i++) {
			int i2 = i+1;
			int val = soundAnalyzer->getWaveLeft((int)(waveStart + i*waveLength))*waveAmplitude;
			int val2 = soundAnalyzer->getWaveLeft((int)(waveStart + i2*waveLength))*waveAmplitude;
			if(val > videoDimX) val = videoDimX; if(val < -videoDimX) val = -videoDimX;
			if(val2 > videoDimX) val2 = videoDimX; if(val2 < -videoDimX) val2 = -videoDimX;
			
			int y = (videoDimY-1) - waveOffset;
			int yOutline1 = y + videoOversample;
			int yOutline2 = y - videoOversample*2;
			for(int j=0; j<videoOversample*2; j++) videoDriver->drawLine(Vector(i, (y-j) + val), color1, Vector(i2, (y-j) + val2), color1);
			for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector(i, (yOutline1-j) + val), Color(0,0,0), Vector(i2, (yOutline1-j) + val2), Color(0,0,0));
			for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector(i, (yOutline2-j) + val), Color(0,0,0), Vector(i2, (yOutline2-j) + val2), Color(0,0,0));
		}
		
		//right wave
		if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
			for(int i=0; i<videoDimX; i++) {
				int i2 = i+1;
				int val = -soundAnalyzer->getWaveRight((int)(waveStart + i*waveLength))*waveAmplitude;
				int val2 = -soundAnalyzer->getWaveRight((int)(waveStart + i2*waveLength))*waveAmplitude;
				if(val > videoDimX) val = videoDimX; if(val < -videoDimX) val = -videoDimX;
				if(val2 > videoDimX) val2 = videoDimX; if(val2 < -videoDimX) val2 = -videoDimX;
				
				int y = waveOffset;
				int yOutline1 = y - videoOversample;
				int yOutline2 = y + videoOversample*2;
				for(int j=0; j<videoOversample*2; j++) videoDriver->drawLine(Vector(i, (y+j) + val), color2, Vector(i2, (y+j) + val2), color2);
				for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector(i, (yOutline1+j) + val), Color(0,0,0), Vector(i2, (yOutline1+j) + val2), Color(0,0,0));
				for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector(i, (yOutline2+j) + val), Color(0,0,0), Vector(i2, (yOutline2+j) + val2), Color(0,0,0));
			}
		}
	}
	
	//spectrum with black outline
	if(style==STYLE_FULL || style==STYLE_NO_WAVE) {
		float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)videoDimX);
		float specAmplitude = 0.05f*((float)videoDimY/1000.0f);
		for(int i=0; i<videoDimX; i++) {
			int valL = soundAnalyzer->getSpecLeft((int)(i*specLength))*specAmplitude;
			int valR = soundAnalyzer->getSpecRight((int)(i*specLength))*specAmplitude;
			if(valL < videoOversample) valL = videoOversample; if(valL > videoDimY) valL = videoDimY;
			if(valR < videoOversample) valR = videoOversample; if(valR > videoDimY) valR = videoDimY;
			int yCenter = videoDimY/2;
			
			//outline
			for(int j=0; j<videoOversample; j++) {
				videoDriver->drawPoint(Vector(i, yCenter - (valL+1+j)), Color(0,0,0));
				videoDriver->drawPoint(Vector(i, (yCenter-1) + (valR+1+j)), Color(0,0,0));
			}
			
			//spec
			Color colorS = color1; 
			colorS.lerp(color2, 50);
			colorS.lerp(Color(220,220,220), 90);
			if(style==STYLE_NO_WAVE) {
				colorS = color1; 
				colorS.lerp(Color(220,220,220), 20);
			}
			videoDriver->drawLine(Vector(i, yCenter - valL), colorS, Vector(i, (yCenter-1) + valR), colorS);
		}
	}
}

//! Sets the visualizer colors
void CStraightVisualizer::setColors(Color color1, Color color2)
{
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CStraightVisualizer::getColor(int num)
{
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CStraightVisualizer::setStyle(int style)
{
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	this->style = style;
}

//! Gets the visualizer style
int CStraightVisualizer::getStyle()
{
	return style;
}

//! Clears all resources of the visualizer
void CStraightVisualizer::clear()
{
	
}
