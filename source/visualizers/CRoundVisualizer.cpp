//-----------------------------------------------------------------------------------------
// Title:	Round Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CRoundVisualizer.h"
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
#include <math.h>
#include <stdio.h>

#define STYLE_FULL 0
#define STYLE_NO_WAVE 1
#define STYLE_NO_SPEC 2
#define STYLE_JUST_SPEC 3
#define NUM_STYLES 4

//! Main Constructor
CRoundVisualizer::CRoundVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa, int r) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0), rotation(r), accRotate(0)
{
}

//! Destructor
CRoundVisualizer::~CRoundVisualizer()
{
	clear();
}

//! Sets up the visualizer
void CRoundVisualizer::setup()
{
	videoDriver->setRotation(rotation);
	soundAnalyzer->setSamplingFrequency(SND_DEFAULT_SAMPLE_FREQUENCY);
	soundAnalyzer->setWaveLPF(0.3f);
	soundAnalyzer->setWaveTimeSmooth(0.6f);
	soundAnalyzer->setSpecSmoothPass(8);
	soundAnalyzer->setSpecTimeSmooth(0.3f);
}

//! Draws the visualizer
void CRoundVisualizer::draw(double elapsedTime)
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
	int gradientSize = videoDimX/5 + (int)((float)(videoDimX/20)*cappedIntensity);
	videoDriver->drawQuad(Vector(0, 0), colorG, 
		Vector(gradientSize, 0), Color(0,0,0), 
		Vector(gradientSize, videoDimY-1), Color(0,0,0),
		Vector(0, videoDimY-1), colorG);
	videoDriver->drawQuad(Vector(videoDimX - gradientSize, 0), Color(0,0,0), 
		Vector(videoDimX-1, 0), colorG, 
		Vector(videoDimX-1, videoDimY-1), colorG,
		Vector(videoDimX - gradientSize, videoDimY-1), Color(0,0,0));
		
	//waves with black outlines
	if(style==STYLE_FULL || style==STYLE_NO_SPEC) {
		int waveOffset = videoDimX/6;
		int waveStart = 20;
		float waveLength = 0.2f*(((float)SND_BUFFER_SAMPLE_SIZE/(float)videoDimY)/1);
		float waveAmplitude = 0.02f*((float)videoDimX/1000.0f);
		if(color2.Red==0 && color2.Green==0 && color2.Blue==0) {
			waveOffset = videoDimX/2;
			waveAmplitude *= 1.5f;
		}
		
		//left wave
		for(int i=0; i<videoDimY; i++) {
			int i2 = i+1;
			int val = soundAnalyzer->getWaveLeft((int)(waveStart + i*waveLength))*waveAmplitude;
			int val2 = soundAnalyzer->getWaveLeft((int)(waveStart + i2*waveLength))*waveAmplitude;
			if(val > videoDimX) val = videoDimX; if(val < -videoDimX) val = -videoDimX;
			if(val2 > videoDimX) val2 = videoDimX; if(val2 < -videoDimX) val2 = -videoDimX;
			
			int x = (videoDimX-1) - waveOffset;
			int xOutline1 = x + videoOversample;
			int xOutline2 = x - videoOversample*2;
			for(int j=0; j<videoOversample*2; j++) videoDriver->drawLine(Vector((x-j) + val, i), color1, Vector((x-j) + val2, i2), color1);
			for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector((xOutline1-j) + val, i), Color(0,0,0), Vector((xOutline1-j) + val2, i2), Color(0,0,0));
			for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector((xOutline2-j) + val, i), Color(0,0,0), Vector((xOutline2-j) + val2, i2), Color(0,0,0));
		}
		
		//right wave
		if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
			for(int i=0; i<videoDimY; i++) {
				int i2 = i+1;
				int val = -soundAnalyzer->getWaveRight((int)(waveStart + i*waveLength))*waveAmplitude;
				int val2 = -soundAnalyzer->getWaveRight((int)(waveStart + i2*waveLength))*waveAmplitude;
				if(val > videoDimX) val = videoDimX; if(val < -videoDimX) val = -videoDimX;
				if(val2 > videoDimX) val2 = videoDimX; if(val2 < -videoDimX) val2 = -videoDimX;
				
				int y = (videoDimY-1)-i;
				int y2 = (videoDimY-1)-i2;
				int x = waveOffset;
				int xOutline1 = x - videoOversample;
				int xOutline2 = x + videoOversample*2;
				for(int j=0; j<videoOversample*2; j++) videoDriver->drawLine(Vector((x+j) + val, y), color2, Vector((x+j) + val2, y2), color2);
				for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector((xOutline1+j) + val, y), Color(0,0,0), Vector((xOutline1+j) + val2, y2), Color(0,0,0));
				for(int j=0; j<videoOversample; j++) videoDriver->drawLine(Vector((xOutline2+j) + val, y), Color(0,0,0), Vector((xOutline2+j) + val2, y2), Color(0,0,0));
			}
		}
	}
	
	//circle spectrum with black outline
	if(style==STYLE_FULL || style==STYLE_NO_WAVE || style==STYLE_JUST_SPEC) {
		int specPoints = 100;
		int specRadius = fmin((float)videoDimX/2.5f, (float)videoDimY/2.5f);
		float specAngleOffset = M_PI*0.35f + ((float)rotation/180.0f)*M_PI;
		float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)specPoints);
		float specAmplitudeL = 0.13f*((float)specRadius/1000.0f); //outside
		float specAmplitudeR = 0.09f*((float)specRadius/1000.0f); //inside
		int specInnerMax = specRadius - (videoOversample);
		int specOuterMax = videoDimY;
		
		for(int i=0; i<specPoints; i++) {
			int i2 = i+1;
			float angle = ((M_PI*2.0f*i)/specPoints) - specAngleOffset;
			float angle2 = ((M_PI*2.0f*i2)/specPoints) - specAngleOffset;
			int valL = soundAnalyzer->getSpecLeft((int)(i*specLength))*specAmplitudeL;
			int valR = soundAnalyzer->getSpecRight((int)(i*specLength))*specAmplitudeR;
			int valL2 = soundAnalyzer->getSpecLeft((int)(i2*specLength))*specAmplitudeL;
			int valR2 = soundAnalyzer->getSpecRight((int)(i2*specLength))*specAmplitudeR;
			if(valL < videoOversample) valL = videoOversample;
			if(valL2 < videoOversample) valL2 = videoOversample;
			if(valL > specOuterMax) valL = specOuterMax; if(valL2 > specOuterMax) valL2 = specOuterMax;
			if(valR > specInnerMax) valR = specInnerMax; if(valR2 > specInnerMax) valR2 = specInnerMax;
			Vector center = Vector(videoDimX/2, videoDimY/2);
			
			//outline
			for(int j=0; j<videoOversample; j++) {
				videoDriver->drawLine(center + Vector(cos(angle)*(specRadius+valL+1+j), sin(angle)*(specRadius+valL+1+j)), Color(0,0,0),
					center + Vector(cos(angle2)*(specRadius+valL2+1+j), sin(angle2)*(specRadius+valL2+1+j)), Color(0,0,0));
			}
			
			//spec
			Color colorS = color1; 
			colorS.lerp(color2, 50);
			colorS.lerp(Color(220,220,220), 90);
			if(style==STYLE_JUST_SPEC) {
				colorS = color1; 
				colorS.lerp(Color(220,220,220), 20);
			}
			videoDriver->drawQuad(center + Vector(cos(angle)*(specRadius+valL), sin(angle)*(specRadius+valL)), colorS,
				center + Vector(cos(angle)*(specRadius-valR), sin(angle)*(specRadius-valR)), colorS,
				center + Vector(cos(angle2)*(specRadius-valR2), sin(angle2)*(specRadius-valR2)), colorS,
				center + Vector(cos(angle2)*(specRadius+valL2), sin(angle2)*(specRadius+valL2)), colorS);
		}
	}
	
	//center accent with black outline
	if(style==STYLE_FULL || style==STYLE_NO_WAVE || style==STYLE_NO_SPEC) {
		accRotate += (0.0f + intensity*2.0f)*elapsedTime;
		while(accRotate > M_PI*2.0f) accRotate -= M_PI*2.0f;
		int accRadius = fmin((float)videoDimX/5.0f + 0.2f*(float)videoDimX*bassIntensity, (float)videoDimY/5.0f + 0.2f*(float)videoDimY*bassIntensity);
		if(style==STYLE_NO_SPEC) accRadius = 1.5f*(float)accRadius;
		Vector center = Vector(videoDimX/2, videoDimY/2);
		
		//outline
		for(int i=0; i<videoOversample; i++) {
			videoDriver->drawLine(center + Vector(cos(accRotate)*(accRadius+1+i), sin(accRotate)*(accRadius+1+i)), Color(0,0,0), 
				center + Vector(cos(accRotate+(M_PI*0.5f))*(accRadius+1+i), sin(accRotate+(M_PI*0.5f))*(accRadius+1+i)), Color(0,0,0));
			videoDriver->drawLine(center + Vector(cos(accRotate+(M_PI*0.5f))*(accRadius+1+i), sin(accRotate+(M_PI*0.5f))*(accRadius+1+i)), Color(0,0,0), 
				center + Vector(cos(accRotate+(M_PI*1.0f))*(accRadius+1+i), sin(accRotate+(M_PI*1.0f))*(accRadius+1+i)), Color(0,0,0));
			videoDriver->drawLine(center + Vector(cos(accRotate+(M_PI*1.0f))*(accRadius+1+i), sin(accRotate+(M_PI*1.0f))*(accRadius+1+i)), Color(0,0,0), 
				center + Vector(cos(accRotate+(M_PI*1.5f))*(accRadius+1+i), sin(accRotate+(M_PI*1.5f))*(accRadius+1+i)), Color(0,0,0));
			videoDriver->drawLine(center + Vector(cos(accRotate+(M_PI*1.5f))*(accRadius+1+i), sin(accRotate+(M_PI*1.5f))*(accRadius+1+i)), Color(0,0,0), 
				center + Vector(cos(accRotate+(M_PI*2.0f))*(accRadius+1+i), sin(accRotate+(M_PI*2.0f))*(accRadius+1+i)), Color(0,0,0));
		}
		
		//accent
		videoDriver->drawQuad(center + Vector(cos(accRotate)*(accRadius), sin(accRotate)*(accRadius)), color2,
			center + Vector(cos(accRotate+(M_PI*0.5f))*(accRadius), sin(accRotate+(M_PI*0.5f))*(accRadius)), color1,
			center + Vector(cos(accRotate+(M_PI*1.0f))*(accRadius), sin(accRotate+(M_PI*1.0f))*(accRadius)), color2,
			center + Vector(cos(accRotate+(M_PI*1.5f))*(accRadius), sin(accRotate+(M_PI*1.5f))*(accRadius)), color1);
	}
}

//! Sets the visualizer colors
void CRoundVisualizer::setColors(Color color1, Color color2)
{
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CRoundVisualizer::getColor(int num)
{
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CRoundVisualizer::setStyle(int style)
{
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	this->style = style;
}

//! Gets the visualizer style
int CRoundVisualizer::getStyle()
{
	return style;
}

//! Clears all resources of the visualizer
void CRoundVisualizer::clear()
{
	
}
