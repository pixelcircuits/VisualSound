//-----------------------------------------------------------------------------------------
// Title:	Video Driver
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include "Vector.h"
#include "Color.h"

//! Class that performs all the rendering to the raw data buffer (8 bit encoding)
class CVideoDriver
{
public:
	//! Main constructor
	CVideoDriver(Vector size, unsigned int oversample);

	//! Destructor
	~CVideoDriver();

	//! Clears the video buffer to the specified color
	void clearVideoBuffer(Color color);

	//! Sets the brightness value from 1 to 100 (full)
	void setBrightness(char bright);

	//! Gets the brightness value
	char getBrightness() const;
	
	//! Gets the dimension of the video buffer (size*overSample)
	Vector getDimension() const;
	
	//! Gets the oversample value (width of one pixel)
	unsigned int getOversample() const;

	//! Draws a point
	void drawPoint(Vector pos, Color color);

	//! Draws a line
	void drawLine(Vector pos0, Color color0, Vector pos1, Color color1);

	//! Draws a triangle
	void drawTri(Vector pos0, Color color0, Vector pos1, Color color1, Vector pos2, Color color2);

	//! Draws a quad
	void drawQuad(Vector pos0, Color color0, Vector pos1, Color color1, Vector pos2, Color color2, Vector pos3, Color color3);
	
	//! Post process brightness multiplier
	void ppBrightness(float multiplier);

	//! Sends the video data to the display
	void flush();

private:
	unsigned char* videoBuffer;
	unsigned int videoOversample;
	Vector videoDimension;

	//! Gets color at given point
	Color getPoint(Vector pos);
	
	//! Draws a line and records the outline to memory
	void drawLine_mem(Vector pos0, Color color0, Vector pos1, Color color1, int format, int mem[][2][2], Color memC[][2]);
	
	//! Records the correct data into the memory arrays
	void record_mem(int x, int y, int z, Color color, int format, int mem[][2][2], Color memC[][2]);
	
	//! Determines color blend from given data
	Color blendColor(Color color0, Color color1, int start, int end, int value);
};

#endif
