//-----------------------------------------------------------------------------------------
// Title:	Particle Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CParticleVisualizer.h"
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STYLE_RAIN 0
#define STYLE_FIREWORKS 1
#define STYLE_SWIRL 2
#define NUM_STYLES 3

#define TARGET_FPS 50

//! Main Constructor
CParticleVisualizer::CParticleVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0)
{
}

//! Destructor
CParticleVisualizer::~CParticleVisualizer()
{
	clear();
}

//! Sets up the visualizer
void CParticleVisualizer::setup()
{
	soundAnalyzer->setSamplingFrequency(SND_DEFAULT_SAMPLE_FREQUENCY);
	soundAnalyzer->setWaveLPF(0.3f);
	soundAnalyzer->setWaveTimeSmooth(0.6f);
	soundAnalyzer->setSpecSmoothPass(1);
	soundAnalyzer->setSpecTimeSmooth(0.3f);
	
	//clear data
	for(int i=0; i<MAX_PARTICLES; i++) {
		particlePos[i][0] = 0; particlePos[i][1] = 0; particlePos[i][2] = 0;
		particleVel[i][0] = 0; particleVel[i][1] = 0; particleVel[i][2] = 0;
		particleVal[i] = 0;
		particleVal2[i] = 0;
		particleActive[i] = false;
	}
}

//! Draws the visualizer
void CParticleVisualizer::draw(double elapsedTime)
{
	//compute basic values
	float intensity = (float)(soundAnalyzer->getVURight()+soundAnalyzer->getVULeft())/6000.0f;
	float bassIntensity = (float)(soundAnalyzer->getBassRight()+soundAnalyzer->getBassLeft())/24000.0f;
	float cappedIntensity = (intensity > 1.0f) ? 1.0f : intensity;
	int videoOversample = videoDriver->getOversample();
	int videoDimX = videoDriver->getDimension().X;
	int videoDimY = videoDriver->getDimension().Y;
	int videoDimZ = videoDriver->getDimension().Z;
	
	//get consistent frame rate
	double timeLeft = (1.0/(double)TARGET_FPS) - elapsedTime;
	if(timeLeft > 0) {		
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = (long)(timeLeft*1000.0*1000.0*1000.0);
		nanosleep(&ts, &ts);
	}
	
	//fade buffer
	videoDriver->ppBrightness(0.8f);
	
	//rain style
	if(style == STYLE_RAIN) {
		//update particle velocity
		const float gravity = 0.02;
		const float termVel = 0.5;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {		
			//gravity
			particleVel[i][1] -= gravity;
			
			//terminal velocity
			float adjTermVel = (((particleVal2[i]*0.8)+20)*termVel)/100.0;
			if(particleVel[i][0] > adjTermVel) particleVel[i][0] = adjTermVel;
			if(particleVel[i][1] > adjTermVel) particleVel[i][1] = adjTermVel;
			if(particleVel[i][2] > adjTermVel) particleVel[i][2] = adjTermVel;
			if(particleVel[i][0] < -adjTermVel) particleVel[i][0] = -adjTermVel;
			if(particleVel[i][1] < -adjTermVel) particleVel[i][1] = -adjTermVel;
			if(particleVel[i][2] < -adjTermVel) particleVel[i][2] = -adjTermVel;
		}
		
		//update particle position
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {
			particlePos[i][0] += particleVel[i][0];
			particlePos[i][1] += particleVel[i][1];
			particlePos[i][2] += particleVel[i][2];
		}
		
		//cleanup particles
		for(int i=0; i<MAX_PARTICLES; i++) {
			if(particleActive[i] && particlePos[i][1] < 0) {
				particleActive[i] = false;
			}
		}
		
		//create new particles
		int targetNumParticles = 30 + cappedIntensity*60;
		int numActiveParticles = 0;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) numActiveParticles++;
		for(int i=0; i<MAX_PARTICLES && numActiveParticles < targetNumParticles; i++) if(!particleActive[i]) {
			particlePos[i][0] = rand()%(videoDimX/videoOversample);
			particlePos[i][1] = rand()%(videoDimY/videoOversample)*2 + (videoDimY/videoOversample) + 2;
			particlePos[i][2] = rand()%(videoDimZ/videoOversample);
			particleVel[i][0] = 0;
			particleVel[i][1] = 0;
			particleVel[i][2] = 0;
			
			particleVal[i] = rand()%100; //color
			particleVal2[i] = rand()%100; //weight
			particleActive[i] = true;
			numActiveParticles++;
		}
		
		//draw
		Color highlight = Color(250,250,250);
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {
			Color c = color1;
			if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
				c.lerp(color2, particleVal[i]);
			}
			const double percentColor = 40.0;
			if(particleVal2[i] > percentColor) {
				c.lerp(highlight, 100-((particleVal2[i]-percentColor)*(100.0/(100.0-percentColor))));
			}
			for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++){
				videoDriver->drawPoint(Vector((int)particlePos[i][0]*videoOversample, (int)particlePos[i][1]*videoOversample, (int)particlePos[i][2]*videoOversample) + Vector(x,y,z), c);
			}
		}
	}

	//fireworks style
	if(style == STYLE_FIREWORKS) {
		//update particle velocity
		const double launchTime = 0.69;
		const double lifeTime = -1.3;
		const float gravity = 0.0075*videoOversample;
		const float termVel = 0.75*videoOversample;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i] && particleVal[i] < launchTime) {
			//gravity
			particleVel[i][1] -= gravity;
			
			//terminal velocity
			if(particleVel[i][0] > termVel) particleVel[i][0] = termVel;
			if(particleVel[i][1] > termVel) particleVel[i][1] = termVel;
			if(particleVel[i][2] > termVel) particleVel[i][2] = termVel;
			if(particleVel[i][0] < -termVel) particleVel[i][0] = -termVel;
			if(particleVel[i][1] < -termVel) particleVel[i][1] = -termVel;
			if(particleVel[i][2] < -termVel) particleVel[i][2] = -termVel;
		}
		
		//update particle position
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i] && particleVal[i] < launchTime) {
			particlePos[i][0] += particleVel[i][0];
			particlePos[i][1] += particleVel[i][1];
			particlePos[i][2] += particleVel[i][2];
		}
		
		//explode/cleanup particles
		for(int i=0; i<MAX_PARTICLES; i++) {
			if(particleActive[i]) {
				double oldVal = particleVal[i];
				particleVal[i] -= elapsedTime;
				
				//explode
				const double explode = 0.25;
				if(particleVal[i] <= 0.0 && oldVal > 0.0) {
					particleVel[i][0] = (((float)(rand()%1000)/1000.0f)*explode*2 - explode)*videoOversample;
					particleVel[i][1] = (((float)(rand()%1000)/1000.0f)*explode*2 - explode)*videoOversample;
					particleVel[i][2] = (((float)(rand()%1000)/1000.0f)*explode*2 - explode)*videoOversample;
				}
				
				//cleanup
				if(particleVal[i] < lifeTime) {
					particleActive[i] = false;
				}
			}
		}
		
		//create new particles
		int numParticlesPerBatch = 120;
		int targetNumParticles = numParticlesPerBatch;
		if(cappedIntensity > 0.5) targetNumParticles = numParticlesPerBatch*2;
		if(cappedIntensity > 0.9) targetNumParticles = numParticlesPerBatch*3;
		int numActiveParticles = 0;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) numActiveParticles++;
		while(numActiveParticles < MAX_PARTICLES && numActiveParticles < targetNumParticles) {
			float posX = rand()%videoDimX;
			float posY = -rand()%(videoDimY/2) - videoOversample;
			float posZ = rand()%videoDimZ;
			double life = launchTime + ((float)(rand()%1000)/1000.0f)*1.0f;
			int color = rand()%100;
			int particlesInBatch = 0;
			for(int i=0; i<MAX_PARTICLES && particlesInBatch<numParticlesPerBatch; i++) if(!particleActive[i]) {			
				particlePos[i][0] = posX;
				particlePos[i][1] = posY;
				particlePos[i][2] = posZ;
				particleVel[i][0] = 0;
				particleVel[i][1] = 0.4*videoOversample;
				particleVel[i][2] = 0;
				
				particleVal[i] = life; //lifetime
				particleVal2[i] = color; //color
				particleActive[i] = true;
				numActiveParticles++;
				particlesInBatch++;
			}
		}
		
		//draw
		Color highlight = Color(250,250,250);
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {
			Color c = highlight;
			if(particleVal[i] < 0) {
				c = color1;
				if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
					c.lerp(color2, particleVal2[i]);
				}
				const double highlightBefore = 0.125;
				if(particleVal[i] > lifeTime*highlightBefore) {
					c.lerp(highlight, 100-((particleVal[i]/(lifeTime*highlightBefore))*100));
				}
				const double fadeAfter = 0.4;
				if(particleVal[i] < lifeTime*fadeAfter) {
					c.lerp(Color(0,0,0), ((particleVal[i]-(lifeTime*fadeAfter))/(lifeTime*(1.0-fadeAfter)))*100);
				}
			}
			for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++){
				videoDriver->drawPoint((Vector(particlePos[i][0], particlePos[i][1], particlePos[i][2])/videoOversample)*videoOversample + Vector(x,y,z), c);
			}
		}
	}
	
	//swirl style
	if(style == STYLE_SWIRL) {
		//update rotating point
		accRotate += 0.03f + intensity*0.05f;
		while(accRotate > M_PI*2.0f*6.0f) accRotate -= M_PI*2.0f*6.0f;
		int r = ((sin(accRotate/2)+sin((accRotate/2)*3)/3.0+sin((accRotate/2)*5)/5.0)   +1)/2      *(1*videoOversample)    + (2*videoOversample);
		Vector p = Vector(r, r, r);
		p.rotate(accRotate, accRotate/3, accRotate/1.5);
		p += Vector(5,5,5)*videoOversample - (Vector(1,1,1)*videoOversample)/2;
		
		//update particle velocity
		const float gravity = 0.4*videoOversample;
		const float termVel = 0.4*videoOversample;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {		
			//gravity
			float v[3] = { particlePos[i][0]-p.X, particlePos[i][1]-p.Y, particlePos[i][2]-p.Z };
			float r = (gravity*gravity)/(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
			float g[3] = { v[0]*r, v[1]*r, v[2]*r };
			
			particleVel[i][0] -= g[0];
			particleVel[i][1] -= g[1];
			particleVel[i][2] -= g[2];
			
			//terminal velocity
			if(particleVel[i][0] > termVel) particleVel[i][0] = termVel;
			if(particleVel[i][1] > termVel) particleVel[i][1] = termVel;
			if(particleVel[i][2] > termVel) particleVel[i][2] = termVel;
			if(particleVel[i][0] < -termVel) particleVel[i][0] = -termVel;
			if(particleVel[i][1] < -termVel) particleVel[i][1] = -termVel;
			if(particleVel[i][2] < -termVel) particleVel[i][2] = -termVel;
		}
		
		//update particle position
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {
			particlePos[i][0] += particleVel[i][0];
			particlePos[i][1] += particleVel[i][1];
			particlePos[i][2] += particleVel[i][2];
		}
		
		//cleanup particles
		for(int i=0; i<MAX_PARTICLES; i++) {
			if(particleActive[i]) {
				particleVal[i] -= elapsedTime;
				if(particleVal[i] < 0) {
					particleActive[i] = false;
				}
			}
		}
		
		//create new particles
		int targetNumParticles = 400;
		int numActiveParticles = 0;
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) numActiveParticles++;
		for(int i=0; i<MAX_PARTICLES && numActiveParticles < targetNumParticles; i++) if(!particleActive[i]) {
			particlePos[i][0] = rand()%(videoDimX);
			particlePos[i][1] = rand()%(videoDimY);
			particlePos[i][2] = rand()%(videoDimZ);
			particleVel[i][0] = 0;
			particleVel[i][1] = 0;
			particleVel[i][2] = 0;
			
			particleVal[i] = ((float)(rand()%1000)/1000.0f)*30.0 + 30.0f; //lifetime
			particleVal2[i] = rand()%100; //color
			particleActive[i] = true;
			numActiveParticles++;
		}
		
		//draw
		Color highlight = Color(250,250,250);
		for(int i=0; i<MAX_PARTICLES; i++) if(particleActive[i]) {
			Color c = color1;
			if(color2.Red>0 || color2.Green>0 || color2.Blue>0) {
				c.lerp(color2, particleVal2[i]);
			}
			float d[3] = {abs(particlePos[i][0] - p.X), abs(particlePos[i][1] - p.Y), abs(particlePos[i][2] - p.Z)};
			float distance = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
			float maxDistance = ((float)videoDimX)/3.0;
			if(distance < maxDistance) {
				c.lerp(highlight, (100.0-((distance/maxDistance)*100.0))*0.9 + 0);
			}
			for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++){
				videoDriver->drawPoint((Vector(particlePos[i][0], particlePos[i][1], particlePos[i][2])/videoOversample)*videoOversample + Vector(x,y,z), c);
			}
		}
	}
}

//! Sets the visualizer colors
void CParticleVisualizer::setColors(Color color1, Color color2)
{
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CParticleVisualizer::getColor(int num)
{
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CParticleVisualizer::setStyle(int style)
{
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	if(this->style != style) {
		this->style = style;
		
		for(int i=0; i<MAX_PARTICLES; i++) {
			particleActive[i] = false;
		}
	}
}

//! Gets the visualizer style
int CParticleVisualizer::getStyle()
{
	return style;
}

//! Clears all resources of the visualizer
void CParticleVisualizer::clear()
{
	
}
