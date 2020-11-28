//-----------------------------------------------------------------------------------------
// Title:	Geometry Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef GEOMETRY_VISUALIZER_H
#define GEOMETRY_VISUALIZER_H

#include "../IVisualizer.h"

class CVideoDriver;
class CSoundAnalyzer;

//! Geometry Music Visualizer
class CGeometryVisualizer : public IVisualizer
{
public:
	//! Main Constructor
	CGeometryVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa);

	//! Destructor
	~CGeometryVisualizer();

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
	float accRotate;
};

#endif
