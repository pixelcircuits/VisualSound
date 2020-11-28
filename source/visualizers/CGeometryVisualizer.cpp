//-----------------------------------------------------------------------------------------
// Title:	Geometry Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CGeometryVisualizer.h"
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
#include <math.h>
#include <stdio.h>

#define STYLE_CUBE      1
#define STYLE_PLANES    0
#define STYLE_SPHERE    2
#define NUM_STYLES      3

//! Main Constructor
CGeometryVisualizer::CGeometryVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0), accRotate(0)
{
}

//! Destructor
CGeometryVisualizer::~CGeometryVisualizer()
{
	clear();
}

//! Sets up the visualizer
void CGeometryVisualizer::setup()
{
	soundAnalyzer->setSamplingFrequency(SND_DEFAULT_SAMPLE_FREQUENCY);
	soundAnalyzer->setWaveLPF(0.3f);
	soundAnalyzer->setWaveTimeSmooth(0.6f);
	soundAnalyzer->setSpecSmoothPass(64);
	soundAnalyzer->setSpecTimeSmooth(0.3f);
}

//! Draws the visualizer
void CGeometryVisualizer::draw(double elapsedTime)
{
	//compute basic values
	float intensity = (float)(soundAnalyzer->getVURight()+soundAnalyzer->getVULeft())/6000.0f;
	float bassIntensity = (float)(soundAnalyzer->getBassRight()+soundAnalyzer->getBassLeft())/24000.0f;
	float cappedIntensity = (intensity > 1.0f) ? 1.0f : intensity;
	float cappedBassIntensity = (bassIntensity > 1.0f) ? 1.0f : bassIntensity;
	int videoOversample = videoDriver->getOversample();
	int videoDimX = videoDriver->getDimension().X;
	int videoDimY = videoDriver->getDimension().Y;
	int videoDimZ = videoDriver->getDimension().Z;
	
	//clear buffer
	videoDriver->clearVideoBuffer(Color(0,0,0));
	
	//data for all styles
	Color highlightColor = Color(150,150,150);
	
	//cube style
	if(style == STYLE_CUBE) {
		accRotate += (0.5f + intensity*3.5f)*elapsedTime;
		while(accRotate > M_PI*2.0f) accRotate -= M_PI*2.0f;
		int r = 3*videoOversample;
		Vector p[8] = { Vector(r,r,r), Vector(r,r,-r), Vector(-r,r,-r), Vector(-r,r,r), Vector(r,-r,r), Vector(r,-r,-r), Vector(-r,-r,-r), Vector(-r,-r,r) };
		for(int i=0; i<8; i++) p[i].rotate(accRotate, accRotate, 0.0f);
		for(int i=0; i<8; i++) p[i]+=Vector(5,5,5)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
		for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
			videoDriver->drawLine(p[0]+Vector(x,y,z), color1, p[1]+Vector(x,y,z), color2);
			videoDriver->drawLine(p[1]+Vector(x,y,z), color2, p[2]+Vector(x,y,z), color1);
			videoDriver->drawLine(p[2]+Vector(x,y,z), color1, p[3]+Vector(x,y,z), color2);
			videoDriver->drawLine(p[3]+Vector(x,y,z), color2, p[0]+Vector(x,y,z), color1);
			
			videoDriver->drawLine(p[0]+Vector(x,y,z), color1, p[4]+Vector(x,y,z), color2);
			videoDriver->drawLine(p[1]+Vector(x,y,z), color2, p[5]+Vector(x,y,z), color1);
			videoDriver->drawLine(p[2]+Vector(x,y,z), color1, p[6]+Vector(x,y,z), color2);
			videoDriver->drawLine(p[3]+Vector(x,y,z), color2, p[7]+Vector(x,y,z), color1);
			
			videoDriver->drawLine(p[4]+Vector(x,y,z), color2, p[5]+Vector(x,y,z), color1);
			videoDriver->drawLine(p[5]+Vector(x,y,z), color1, p[6]+Vector(x,y,z), color2);
			videoDriver->drawLine(p[6]+Vector(x,y,z), color2, p[7]+Vector(x,y,z), color1);
			videoDriver->drawLine(p[7]+Vector(x,y,z), color1, p[4]+Vector(x,y,z), color2);
		}
		videoDriver->ppBrightness(1.8f);
	}
	
	//planes style
	if(style == STYLE_PLANES) {
		accRotate += (0.6f + intensity*2.5f)*elapsedTime;
		while(accRotate > (M_PI*2.0f)*2) accRotate -= (M_PI*2.0f)*2;
		int r = videoDimX;
		Vector p1[4] = { Vector(-r,-r,0), Vector(r,-r,0), Vector(r,r,0), Vector(-r,r,0) };
		Vector p2[4] = { Vector(0,-r,-r), Vector(0,r,-r), Vector(0,r,r), Vector(0,-r,r) };
		for(int i=0; i<4; i++) p1[i].rotate(accRotate, accRotate, 0.0f);
		for(int i=0; i<4; i++) p2[i].rotate(accRotate, -accRotate, accRotate);
		if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
			for(int i=0; i<4; i++) p1[i]+=Vector(3,3,3)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
			for(int i=0; i<4; i++) p2[i]+=Vector(7,7,7)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
		} else {
			for(int i=0; i<4; i++) p1[i]+=Vector(5,5,5)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
		}
		for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
			videoDriver->drawQuad(p1[0]+Vector(x,y,z), color1, p1[1]+Vector(x,y,z), color1, p1[2]+Vector(x,y,z), color1, p1[3]+Vector(x,y,z), color1);
			videoDriver->drawQuad(p2[0]+Vector(x,y,z), color2, p2[1]+Vector(x,y,z), color2, p2[2]+Vector(x,y,z), color2, p2[3]+Vector(x,y,z), color2);
		}
	}
	
	//sphere style
	if(style == STYLE_SPHERE) {
		accRotate += (2.5f + intensity*6.5f)*elapsedTime;
		while(accRotate > M_PI*2.0f) accRotate -= M_PI*2.0f;
		
		int radius = (4 + cappedBassIntensity*6)*videoOversample;
		const int steps = 100;
		Vector p[steps*(steps/2)];
		Color c[steps*(steps/2)];
		for(int i=0; i<steps/2; i++) {
			Color cs = color1;
			cs.lerp(color2, (i*100)/(steps/2));
			for(int j=0; j<steps; j++) {
				double angle1 = ((double)i*M_PI*2.0)/(double)steps + (M_PI/2.0);
				double angle2 = ((double)j*M_PI*2.0)/(double)steps;
				int posX = cos(angle1)*radius;
				int posY = sin(angle1)*radius;
				int posZ = sin(angle2)*posX;
				posX = cos(angle2)*posX;

				p[j*(steps/2)+i] = Vector(posX,posY,posZ);
				c[j*(steps/2)+i] = cs;
			}
		}
		
		Vector midPoint = Vector(5,5,5)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
		for(int i=0; i<steps*(steps/2); i++) {
			p[i].rotate(accRotate, accRotate, 0.0f);
			videoDriver->drawPoint(p[i]+midPoint, c[i]);
		}
		videoDriver->ppBrightness(1.8f);
	}
	
	//spectrum floor
	float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)videoDimZ);
	float specAmplitude = 0.09f*((float)videoDimX/1000.0f);
	for(int i=0; i<videoDimZ; i++) {
		int valL = soundAnalyzer->getSpecLeft((int)(i*specLength))*specAmplitude;
		int valR = soundAnalyzer->getSpecRight((int)(i*specLength))*specAmplitude;
		for(int y=0; y<videoOversample; y++) {
			videoDriver->drawLine(Vector((videoDimX/2)+videoOversample/2+valL,0,i)+Vector(0,y,0), highlightColor, Vector(((videoDimX/2)-videoOversample)-valR,0,i)+Vector(0,y,0), highlightColor);
		}
	}
}

//! Sets the visualizer colors
void CGeometryVisualizer::setColors(Color color1, Color color2)
{
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CGeometryVisualizer::getColor(int num)
{
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CGeometryVisualizer::setStyle(int style)
{
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	this->style = style;
}

//! Gets the visualizer style
int CGeometryVisualizer::getStyle()
{
	return style;
}

//! Clears all resources of the visualizer
void CGeometryVisualizer::clear()
{
	
}
