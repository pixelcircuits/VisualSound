//-----------------------------------------------------------------------------------------
// Title:	Music Visualizer Template
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef TEMPLATE_VISUALIZER_H /*** rename according to your visualizer ***/
#define TEMPLATE_VISUALIZER_H /*** rename according to your visualizer ***/

#include "../IVisualizer.h"

class CVideoDriver;
class CSoundAnalyzer;

//! Straight Music Visualizer
class CTemplateVisualizer : public IVisualizer /*** rename according to your visualizer ***/
{
public:
	//! Main Constructor
	CTemplateVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa);

	//! Destructor
	~CTemplateVisualizer();

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
};

#endif
