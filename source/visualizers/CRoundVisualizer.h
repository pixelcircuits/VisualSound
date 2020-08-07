//-----------------------------------------------------------------------------------------
// Title:	Round Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef ROUND_VISUALIZER_H
#define ROUND_VISUALIZER_H

#include "../IVisualizer.h"

class CVideoDriver;
class CSoundAnalyzer;

//! Round Music Visualizer
class CRoundVisualizer : public IVisualizer
{
public:
	//! Main Constructor
	CRoundVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa, int r);

	//! Destructor
	~CRoundVisualizer();

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
	int rotation;
	float accRotate;
};

#endif
