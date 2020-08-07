//-----------------------------------------------------------------------------------------
// Title:	Visualizer Interface
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef I_VISUALIZER_H
#define I_VISUALIZER_H

#include "Color.h"

//! Interface for visualizer scenes
class IVisualizer
{
public:
	//! Sets up the visualizer
	virtual void setup()=0;

	//! Draws the visualizer (given time since last draw in seconds)
	virtual void draw(double elapsedTime)=0;
	
	//! Sets the visualizer colors
	virtual void setColors(Color color1, Color color2)=0;
	
	//! Gets the visualizer colors
	virtual Color getColor(int num)=0;
	
	//! Sets the visualizer style
	virtual void setStyle(int style)=0;
	
	//! Gets the visualizer style
	virtual int getStyle()=0;
	
	//! Clears all resources of the visualizer
	virtual void clear()=0;
	
protected:

};

#endif
