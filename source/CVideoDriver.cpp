//-----------------------------------------------------------------------------------------
// Title:	Video Driver
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CVideoDriver.h"
#include "led.h"
#include <limits.h>

#define ABS(a) (((a)<0) ? -(a) : (a))
#define ZSGN(a) (((a)<0) ? -1 : (a)>0 ? 1 : 0)

//! Main constructor
CVideoDriver::CVideoDriver(Vector size, unsigned int oversample, unsigned int rotation)
{
	videoOversample = oversample;
	videoSize = Vector(size.X, size.Y);
	
	videoBaseRotation = ((rotation%360)/90) * 90;
	calcRotation(rotation);
	
	videoBuffer = new unsigned char[videoDimension.X*videoDimension.Y*3];
	clearVideoBuffer(Color(0,0,0));
}

//! Destructor
CVideoDriver::~CVideoDriver()
{
	delete[] videoBuffer;
	videoBuffer = 0;
}

//! Clears the video buffer to the specified color
void CVideoDriver::clearVideoBuffer(Color color)
{
	for(int i=0; i<videoDimension.X*videoDimension.Y; i++) {
		int index = i*3;
		videoBuffer[index+0] = color.Red;
		videoBuffer[index+1] = color.Green;
		videoBuffer[index+2] = color.Blue;
	}
}

//! Sets the brightness value from 1 to 100 (full)
void CVideoDriver::setBrightness(char bright)
{
	led_setBrightness(bright);
}

//! Gets the brightness value
char CVideoDriver::getBrightness() const
{
	return led_getBrightness();
}

//! Sets the rotation value (0, 90, 180, 270)
void CVideoDriver::setRotation(unsigned int rotation)
{
	videoRotation = ((rotation%360)/90) * 90;
	calcRotation(videoBaseRotation + videoRotation);
}

//! Gets the rotation value (0, 90, 180, 270)
unsigned int CVideoDriver::getRotation() const
{
	return videoRotation;
}
	
//! Gets the dimension of the video buffer (size*overSample)
Vector CVideoDriver::getDimension() const
{
	return videoDimension;
}

//! Gets the oversample value (width of one pixel)
unsigned int CVideoDriver::getOversample() const
{
	return videoOversample;
}

//! Draws a point
void CVideoDriver::drawPoint(Vector pos, Color color)
{
	if(pos.X >= 0 && pos.X < videoDimension.X && pos.Y >= 0 && pos.Y < videoDimension.Y) {
		int index = (pos.Y*videoDimension.X + pos.X)*3;
		videoBuffer[index+0] = color.Red;
		videoBuffer[index+1] = color.Green;
		videoBuffer[index+2] = color.Blue;
	}
}

//! Draws a line
void CVideoDriver::drawLine(Vector pos0, Color color0, Vector pos1, Color color1)
{
    drawLine_mem(pos0, color0, pos1, color1, 0, 0);
}

//! Draws a triangle
void CVideoDriver::drawTri(Vector pos0, Color color0, Vector pos1, Color color1, Vector pos2, Color color2)
{
	//setup outline buffer
	int memMinMax[videoDimension.Y][2];
	Color memMinMaxColor[videoDimension.Y][2];
	for(int y=0; y<videoDimension.Y; y++) {
		memMinMax[y][0] = INT_MAX;
		memMinMax[y][1] = INT_MIN;
	}
	
	//draw outline
	drawLine_mem(pos0, color0, pos1, color1, memMinMax, memMinMaxColor);
	drawLine_mem(pos1, color1, pos2, color2, memMinMax, memMinMaxColor);
	drawLine_mem(pos2, color2, pos0, color0, memMinMax, memMinMaxColor);
	
	//fill in the triangle
	for(int y=0; y<videoDimension.Y; y++) {
		if(memMinMax[y][0] < INT_MAX && memMinMax[y][1] > INT_MIN) {
			Color color0 = memMinMaxColor[y][0];
			Color color1 = memMinMaxColor[y][1];
			
			int dist = memMinMax[y][1]-memMinMax[y][0];
			for(int x=1; x<dist; x++) {
				Color color = color0;
				color.lerp(color1, (x*100)/dist);
				drawPoint(Vector(memMinMax[y][0]+x, y), color);
			}
		}
	}
}

//! Draws a quad
void CVideoDriver::drawQuad(Vector pos0, Color color0, Vector pos1, Color color1, Vector pos2, Color color2, Vector pos3, Color color3)
{
	Vector posC = (pos0 + pos1 + pos2 + pos3)/4;
	Color colorC = (color0/4) + (color1/4) + (color2/4) + (color3/4);
	
	drawTri(pos0, color0, posC, colorC, pos1, color1);
	drawTri(pos1, color1, posC, colorC, pos2, color2);
	drawTri(pos2, color2, posC, colorC, pos3, color3);
	drawTri(pos3, color3, posC, colorC, pos0, color0);
}

//! Sends the video data to the display
void CVideoDriver::flush()
{
	int displayX = videoDimension.X/videoOversample;
	int displayY = videoDimension.Y/videoOversample;
	for(int x=0; x<displayX; x++) {
		for(int y=0; y<displayY; y++) {
			int red = 0;
			int green = 0;
			int blue = 0;
			for(int xp=0; xp<videoOversample; xp++) {
				for(int yp=0; yp<videoOversample; yp++) {
					int vbIndex = ((y*videoOversample + yp)*videoDimension.X + (x*videoOversample + xp))*3;
					red += videoBuffer[vbIndex+0];
					green += videoBuffer[vbIndex+1];
					blue += videoBuffer[vbIndex+2];
				}
			}
			
			red /= (videoOversample*videoOversample);
			green /= (videoOversample*videoOversample);
			blue /= (videoOversample*videoOversample);
			
			//determine how to draw by rotation calculations
			int yr = y, xr = x;
			if(videoMirror.Y > 0) yr = (displayY-1) - y;
			if(videoMirror.X > 0) xr = (displayX-1) - x;
			if(videoXYFlip > 0) led_write(yr, xr, red, green, blue);
			else led_write(xr, yr, red, green, blue);
		}
	}
	
	led_flush();
}

//! Gets color at given point
Color CVideoDriver::getPoint(Vector pos)
{
	if(pos.X >= 0 && pos.X < videoDimension.X && pos.Y >= 0 && pos.Y < videoDimension.Y) {
		int index = (pos.Y*videoDimension.X + pos.X)*3;
		return Color(videoBuffer[index+0], videoBuffer[index+1], videoBuffer[index+2]);
	}
	return Color(0, 0, 0);
}

//! Calculates the mirror and dimension values for a given angle
void CVideoDriver::calcRotation(int angle)
{
	angle = ((angle%360)/90) * 90;
	if(angle == 0 || angle == 180) {
		videoDimension = Vector(videoSize.X*videoOversample, videoSize.Y*videoOversample);
		videoXYFlip = 0;
	} else {
		videoDimension = Vector(videoSize.Y*videoOversample, videoSize.X*videoOversample);
		videoXYFlip = 1;
	}
	if(angle == 90 || angle == 180) {
		videoMirror.Y = 1;
	} else {
		videoMirror.Y = 0;
	}
	if(angle == 270 || angle == 180) {
		videoMirror.X = 1;
	} else {
		videoMirror.X = 0;
	}
}

//! Draws a line and records the outline to memory
void CVideoDriver::drawLine_mem(Vector pos0, Color color0, Vector pos1, Color color1, int mem[][2], Color memc[][2])
{
    int dx = pos1.X - pos0.X;
    int dy = pos1.Y - pos0.Y;

    int ax = ABS(dx) << 1;
    int ay = ABS(dy) << 1;

    int sx = ZSGN(dx);
    int sy = ZSGN(dy);

    int x = pos0.X;
    int y = pos0.Y;

    if(ax >= ay) // x dominant
    {
        int yd = ay - (ax >> 1);
        for(;;) {
			Color color = color1;
			if(pos0.X!=pos1.X) color.lerp(color0, (ABS(x-pos1.X)*100)/ABS(pos0.X-pos1.X));
			else color = color0;
            drawPoint(Vector(x, y), color);
			if(mem && memc && y >= 0 && y < videoDimension.Y) {
				if(x < mem[y][0]) {
					mem[y][0] = x;
					memc[y][0] = color;
				}
				if(x > mem[y][1]) {
					mem[y][1] = x;
					memc[y][1] = color;
				}
			}
            
			if (x == pos1.X) return;
            if (yd >= 0) {
                y += sy;
                yd -= ax;
            }

            x += sx;
            yd += ay;
        }
    }
    else if(ay >= ax) // y dominant
    {
        int xd = ax - (ay >> 1);
        for(;;) {
			Color color = color1;
			if(pos0.Y!=pos1.Y) color.lerp(color0, (ABS(y-pos1.Y)*100)/ABS(pos0.Y-pos1.Y));
			else color = color0;
            drawPoint(Vector(x, y), color);
			if(mem && memc && y >= 0 && y < videoDimension.Y) {
				if(x < mem[y][0]) {
					mem[y][0] = x;
					memc[y][0] = color;
				}
				if(x > mem[y][1]) {
					mem[y][1] = x;
					memc[y][1] = color;
				}
			}
			
            if(y == pos1.Y) return;
            if(xd >= 0) {
                x += sx;
                xd -= ay;
            }

            y += sy;
            xd += ax;
        }
    }
}
