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
	CVideoDriver(Vector size, unsigned int oversample, unsigned int rotation);

	//! Destructor
	~CVideoDriver();

	//! Clears the video buffer to the specified color
	void clearVideoBuffer(Color color);

	//! Sets the brightness value from 1 to 100 (full)
	void setBrightness(char bright);

	//! Gets the brightness value
	char getBrightness() const;

	//! Sets the rotation value (0, 90, 180, 270)
	void setRotation(unsigned int rotation);

	//! Gets the rotation value (0, 90, 180, 270)
	unsigned int getRotation() const;
	
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

	//! Sends the video data to the display
	void flush();

private:
	unsigned char* videoBuffer;
	unsigned int videoOversample;
	Vector videoSize;
	unsigned int videoBaseRotation;
	unsigned int videoRotation;
	unsigned char videoXYFlip;
	Vector videoDimension;
	Vector videoMirror;

	//! Gets color at given point
	Color getPoint(Vector pos);

	//! Calculates the mirror and dimension values for a given angle
	void calcRotation(int angle);
	
	//! Draws a line and records the outline to memory
	void drawLine_mem(Vector pos0, Color color0, Vector pos1, Color color1, int mem[][2], Color memc[][2]);
};

#endif
