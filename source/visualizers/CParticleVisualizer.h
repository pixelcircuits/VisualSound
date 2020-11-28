//-----------------------------------------------------------------------------------------
// Title:	Particle Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef PARTICLE_VISUALIZER_H
#define PARTICLE_VISUALIZER_H

#include "../IVisualizer.h"

#define MAX_PARTICLES 1000

class CVideoDriver;
class CSoundAnalyzer;

//! Particle Music Visualizer
class CParticleVisualizer : public IVisualizer
{
public:
	//! Main Constructor
	CParticleVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa);

	//! Destructor
	~CParticleVisualizer();

	//! Sets up the visualizer
	void setup();

	//! Draws the visualizer
	void draw(double elapsedTime);
	
	//! Sets the visualizer colors
	void setColors(Color color1, Color color2);
	
	//! Gets the visualizer colors
	Color getColor(int num);
	
	//! Sets the visualizer style
	void setStyle(int style);
	
	//! Gets the visualizer style
	int getStyle();
	
	//! Clears all resources of the visualizer
	void clear();
	
protected:
	CVideoDriver* videoDriver;
	CSoundAnalyzer* soundAnalyzer;
	
	Color color1;
	Color color2;
	int style;
	
	float particlePos[MAX_PARTICLES][3];
	float particleVel[MAX_PARTICLES][3];
	double particleVal[MAX_PARTICLES];
	double particleVal2[MAX_PARTICLES];
	bool particleActive[MAX_PARTICLES];
	float accRotate;
};

#endif
