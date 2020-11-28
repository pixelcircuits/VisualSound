//-----------------------------------------------------------------------------------------
// Title:	Wave Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef WAVE_VISUALIZER_H
#define WAVE_VISUALIZER_H

#include "../IVisualizer.h"

class CVideoDriver;
class CSoundAnalyzer;

//! Straight Music Visualizer
class CWaveVisualizer : public IVisualizer
{
public:
	//! Main Constructor
	CWaveVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa);

	//! Destructor
	~CWaveVisualizer();

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
	double captureTime;
	int captureCount;
	short** dataL;
	short** dataR;

	//! Calculates angle from 0 to 1
	double calcAngleRatio(int x, int y, double zm, double offset);
};

#endif
