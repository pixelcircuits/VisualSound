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
#define MAX(a,b) (((a)<(b)) ? (b) : (a))
#define MIN(a,b) (((a)>(b)) ? (b) : (a))

#define MEM_XY_X 0
#define MEM_XY_Y 1
#define MEM_YZ_Y 2
#define MEM_YZ_Z 3
#define MEM_XZ_X 4
#define MEM_XZ_Z 5
#define MEM_MIN 0
#define MEM_MAX 1

//! Main constructor
CVideoDriver::CVideoDriver(Vector size, unsigned int oversample)
{
	videoOversample = oversample;
	videoDimension = Vector(size.X*videoOversample, size.Y*videoOversample, size.Z*videoOversample);
	
	videoBuffer = new unsigned char[videoDimension.X*videoDimension.Y*videoDimension.Z*3];
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
	for(int i=0; i<videoDimension.X*videoDimension.Y*videoDimension.Z; i++) {
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
	if(pos.X >= 0 && pos.X < videoDimension.X && pos.Y >= 0 && pos.Y < videoDimension.Y && pos.Z >= 0 && pos.Z < videoDimension.Z) {
		int index = (pos.Z*videoDimension.Y*videoDimension.X + pos.Y*videoDimension.X + pos.X)*3;
		videoBuffer[index+0] = color.Red;
		videoBuffer[index+1] = color.Green;
		videoBuffer[index+2] = color.Blue;
	}
}

//! Draws a line
void CVideoDriver::drawLine(Vector pos0, Color color0, Vector pos1, Color color1)
{
    drawLine_mem(pos0, color0, pos1, color1, 0, 0, 0);
}

//! Draws a triangle
void CVideoDriver::drawTri(Vector pos0, Color color0, Vector pos1, Color color1, Vector pos2, Color color2)
{
	//determine format
	int size = 0;
	int format = 0;
	int dx = MAX(pos0.X, MAX(pos1.X, pos2.X)) - MIN(pos0.X, MIN(pos1.X, pos2.X));
	int dy = MAX(pos0.Y, MAX(pos1.Y, pos2.Y)) - MIN(pos0.Y, MIN(pos1.Y, pos2.Y));
	int dz = MAX(pos0.Z, MAX(pos1.Z, pos2.Z)) - MIN(pos0.Z, MIN(pos1.Z, pos2.Z));
	if(dx >= dy && dx >= dz) {
		size = videoDimension.X;
		format = (dy >= dz) ? MEM_YZ_Y : MEM_YZ_Z;
	} else if(dy >= dx && dy >= dz) {
		size = videoDimension.Y;
		format = (dx >= dz) ? MEM_XZ_X : MEM_XZ_Z;
	} else if(dz >= dx && dz >= dy) {
		size = videoDimension.Z;
		format = (dx >= dy) ? MEM_XY_X : MEM_XY_Y;
	}
	
	//setup outline buffer
	int memMinMax[size][2][2];
	Color memMinMaxColor[size][2];
	for(int i=0; i<size; i++) {
		memMinMax[i][0][MEM_MIN] = INT_MAX;
		memMinMax[i][0][MEM_MAX] = INT_MIN;
		memMinMax[i][1][MEM_MIN] = INT_MAX;
		memMinMax[i][1][MEM_MAX] = INT_MIN;
	}
	
	//draw outline
	drawLine_mem(pos0, color0, pos1, color1, format, memMinMax, memMinMaxColor);
	drawLine_mem(pos1, color1, pos2, color2, format, memMinMax, memMinMaxColor);
	drawLine_mem(pos2, color2, pos0, color0, format, memMinMax, memMinMaxColor);
	
	//fill in the triangle
	if(format == MEM_XY_X || format == MEM_XY_Y) {
		for(int z=0; z<videoDimension.Z; z++) {
			if(memMinMax[z][0][MEM_MIN] < INT_MAX && memMinMax[z][0][MEM_MAX] > INT_MIN && memMinMax[z][1][MEM_MIN] < INT_MAX && memMinMax[z][1][MEM_MAX] > INT_MIN) {
				drawLine(Vector(memMinMax[z][0][MEM_MIN],memMinMax[z][1][MEM_MIN],z), memMinMaxColor[z][MEM_MIN], Vector(memMinMax[z][0][MEM_MAX],memMinMax[z][1][MEM_MAX],z), memMinMaxColor[z][MEM_MAX]);
			}
		}
	} else if(format == MEM_YZ_Y || format == MEM_YZ_Z) {
		for(int x=0; x<videoDimension.X; x++) {
			if(memMinMax[x][0][MEM_MIN] < INT_MAX && memMinMax[x][0][MEM_MAX] > INT_MIN && memMinMax[x][1][MEM_MIN] < INT_MAX && memMinMax[x][1][MEM_MAX] > INT_MIN) {
				drawLine(Vector(x,memMinMax[x][0][MEM_MIN],memMinMax[x][1][MEM_MIN]), memMinMaxColor[x][MEM_MIN], Vector(x,memMinMax[x][0][MEM_MAX],memMinMax[x][1][MEM_MAX]), memMinMaxColor[x][MEM_MAX]);
			}
		}
	} else if(format == MEM_XZ_X || format == MEM_XZ_Z) {
		for(int y=0; y<videoDimension.Y; y++) {
			if(memMinMax[y][0][MEM_MIN] < INT_MAX && memMinMax[y][0][MEM_MAX] > INT_MIN && memMinMax[y][1][MEM_MIN] < INT_MAX && memMinMax[y][1][MEM_MAX] > INT_MIN) {
				drawLine(Vector(memMinMax[y][0][MEM_MIN],y,memMinMax[y][1][MEM_MIN]), memMinMaxColor[y][MEM_MIN], Vector(memMinMax[y][0][MEM_MAX],y,memMinMax[y][1][MEM_MAX]), memMinMaxColor[y][MEM_MAX]);
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
	
//! Post process brightness multiplier
void CVideoDriver::ppBrightness(float multiplier)
{
	int displayX = videoDimension.X/videoOversample;
	int displayY = videoDimension.Y/videoOversample;
	int displayZ = videoDimension.Z/videoOversample;
	for(int x=0; x<displayX; x++) {
		for(int y=0; y<displayY; y++) {
			for(int z=0; z<displayZ; z++) {
				int red = 0; int green = 0;	int blue = 0;
				
				for(int xp=0; xp<videoOversample; xp++) {
					for(int yp=0; yp<videoOversample; yp++) {
						for(int zp=0; zp<videoOversample; zp++) {
							int vbIndex = ((z*videoOversample + zp)*videoDimension.Y*videoDimension.X + (y*videoOversample + yp)*videoDimension.X + (x*videoOversample + xp))*3;
							red += videoBuffer[vbIndex+0];
							green += videoBuffer[vbIndex+1];
							blue += videoBuffer[vbIndex+2];
						}
					}
				}
				
				red = (red*multiplier)/(videoOversample*videoOversample*videoOversample);
				green = (green*multiplier)/(videoOversample*videoOversample*videoOversample);
				blue = (blue*multiplier)/(videoOversample*videoOversample*videoOversample);
				
				float maxr = MAX((float)red/255.0f, MAX((float)green/255.0f, (float)blue/255.0f));
				if(maxr > 1.0f) {
					red = (float)red/maxr;
					blue = (float)blue/maxr;
					green = (float)green/maxr;
				}
				
				
				for(int xp=0; xp<videoOversample; xp++) {
					for(int yp=0; yp<videoOversample; yp++) {
						for(int zp=0; zp<videoOversample; zp++) {
							int vbIndex = ((z*videoOversample + zp)*videoDimension.Y*videoDimension.X + (y*videoOversample + yp)*videoDimension.X + (x*videoOversample + xp))*3;
							videoBuffer[vbIndex+0] = red;
							videoBuffer[vbIndex+1] = green;
							videoBuffer[vbIndex+2] = blue;
						}
					}
				}
			}
		}
	}
}

//! Sends the video data to the display
void CVideoDriver::flush()
{
	int displayX = videoDimension.X/videoOversample;
	int displayY = videoDimension.Y/videoOversample;
	int displayZ = videoDimension.Z/videoOversample;
	for(int x=0; x<displayX; x++) {
		for(int y=0; y<displayY; y++) {
			for(int z=0; z<displayZ; z++) {
				int red = 0;
				int green = 0;
				int blue = 0;
				for(int xp=0; xp<videoOversample; xp++) {
					for(int yp=0; yp<videoOversample; yp++) {
						for(int zp=0; zp<videoOversample; zp++) {
							int vbIndex = ((z*videoOversample + zp)*videoDimension.Y*videoDimension.X + (y*videoOversample + yp)*videoDimension.X + (x*videoOversample + xp))*3;
							red += videoBuffer[vbIndex+0];
							green += videoBuffer[vbIndex+1];
							blue += videoBuffer[vbIndex+2];
						}
					}
				}
				
				red /= (videoOversample*videoOversample*videoOversample);
				green /= (videoOversample*videoOversample*videoOversample);
				blue /= (videoOversample*videoOversample*videoOversample);
				
				led_write(x, y, z, red, green, blue);
			}
		}
	}
	
	led_flush();
}

//! Gets color at given point
Color CVideoDriver::getPoint(Vector pos)
{
	if(pos.X >= 0 && pos.X < videoDimension.X && pos.Y >= 0 && pos.Y < videoDimension.Y && pos.Z >= 0 && pos.Z < videoDimension.Z) {
		int index = (pos.Z*videoDimension.Y*videoDimension.X + pos.Y*videoDimension.X + pos.X)*3;
		return Color(videoBuffer[index+0], videoBuffer[index+1], videoBuffer[index+2]);
	}
	return Color(0, 0, 0);
}

//! Draws a line and records the outline to memory
void CVideoDriver::drawLine_mem(Vector pos0, Color color0, Vector pos1, Color color1, int format, int mem[][2][2], Color memC[][2])
{
	int dx = pos1.X - pos0.X;
	int dy = pos1.Y - pos0.Y;
	int dz = pos1.Z - pos0.Z;

	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int az = ABS(dz) << 1;

	int sx = ZSGN(dx);
	int sy = ZSGN(dy);
	int sz = ZSGN(dz);

	int x = pos0.X;
	int y = pos0.Y;
	int z = pos0.Z;

	if(ax >= ay && ax >= az) { // x dominant
		int yd = ay - (ax >> 1);
		int zd = az - (ax >> 1);
		for(;;) {
			Color color = blendColor(color0, color1, pos0.X, pos1.X, x);
			record_mem(x, y, z, color, format, mem, memC);
			drawPoint(Vector(x, y, z), color);

			if (x == pos1.X) return;
			if (yd >= 0) {
				y += sy;
				yd -= ax;
			}
			if (zd >= 0) {
				z += sz;
				zd -= ax;
			}

			x += sx;
			yd += ay;
			zd += az;
		}
	}
	else if(ay >= ax && ay >= az) // y dominant
	{
		int xd = ax - (ay >> 1);
		int zd = az - (ay >> 1);
		for(;;) {
			Color color = blendColor(color0, color1, pos0.Y, pos1.Y, y);
			record_mem(x, y, z, color, format, mem, memC);
			drawPoint(Vector(x, y, z), color);

			if(y == pos1.Y) return;
			if(xd >= 0) {
				x += sx;
				xd -= ay;
			}
			if(zd >= 0) {
				z += sz;
				zd -= ay;
			}

			y += sy;
			xd += ax;
			zd += az;
		}
	}
	else if(az >= ax && az >= ay) // z dominant
	{
		int xd = ax - (az >> 1);
		int yd = ay - (az >> 1);
		for(;;) {
			Color color = blendColor(color0, color1, pos0.Z, pos1.Z, z);
			record_mem(x, y, z, color, format, mem, memC);
			drawPoint(Vector(x, y, z), color);
			
			if(z == pos1.Z) return;
			if(xd >= 0) {
				x += sx;
				xd -= az;
			}
			if(yd >= 0) {
				y += sy;
				yd -= az;
			}

			z += sz;
			xd += ax;
			yd += ay;
		}
	}
}

//! Records the correct data into the memory arrays
void CVideoDriver::record_mem(int x, int y, int z, Color color, int format, int mem[][2][2], Color memC[][2])
{
	if(mem && memC) {
		if(format == MEM_XY_X) {
			if(z >= 0 && z < videoDimension.Z) {
				if(x < mem[z][0][MEM_MIN]) {
					mem[z][0][MEM_MIN] = x;
					mem[z][1][MEM_MIN] = y;
					memC[z][MEM_MIN] = color;
				}
				if(x > mem[z][0][MEM_MAX]) {
					mem[z][0][MEM_MAX] = x;
					mem[z][1][MEM_MAX] = y;
					memC[z][MEM_MAX] = color;
				}
			}
			
		} else if(format == MEM_XY_Y) {
			if(z >= 0 && z < videoDimension.Z) {
				if(y < mem[z][1][MEM_MIN]) {
					mem[z][0][MEM_MIN] = x;
					mem[z][1][MEM_MIN] = y;
					memC[z][MEM_MIN] = color;
				}
				if(y > mem[z][1][MEM_MAX]) {
					mem[z][0][MEM_MAX] = x;
					mem[z][1][MEM_MAX] = y;
					memC[z][MEM_MAX] = color;
				}
			}
			
		} else if(format == MEM_YZ_Y) {
			if(x >= 0 && x < videoDimension.X) {
				if(y < mem[x][0][MEM_MIN]) {
					mem[x][0][MEM_MIN] = y;
					mem[x][1][MEM_MIN] = z;
					memC[x][MEM_MIN] = color;
				}
				if(y > mem[x][0][MEM_MAX]) {
					mem[x][0][MEM_MAX] = y;
					mem[x][1][MEM_MAX] = z;
					memC[x][MEM_MAX] = color;
				}
			}
			
		} else if(format == MEM_YZ_Z) {
			if(x >= 0 && x < videoDimension.X) {
				if(z < mem[x][1][MEM_MIN]) {
					mem[x][0][MEM_MIN] = y;
					mem[x][1][MEM_MIN] = z;
					memC[x][MEM_MIN] = color;
				}
				if(z > mem[x][1][MEM_MAX]) {
					mem[x][0][MEM_MAX] = y;
					mem[x][1][MEM_MAX] = z;
					memC[x][MEM_MAX] = color;
				}
			}
			
		} else if(format == MEM_XZ_X) {
			if(y >= 0 && y < videoDimension.Y) {
				if(x < mem[y][0][MEM_MIN]) {
					mem[y][0][MEM_MIN] = x;
					mem[y][1][MEM_MIN] = z;
					memC[y][MEM_MIN] = color;
				}
				if(x > mem[y][0][MEM_MAX]) {
					mem[y][0][MEM_MAX] = x;
					mem[y][1][MEM_MAX] = z;
					memC[y][MEM_MAX] = color;
				}
			}
			
		} else if(format == MEM_XZ_Z) {
			if(y >= 0 && y < videoDimension.Y) {
				if(z < mem[y][1][MEM_MIN]) {
					mem[y][0][MEM_MIN] = x;
					mem[y][1][MEM_MIN] = z;
					memC[y][MEM_MIN] = color;
				}
				if(z > mem[y][1][MEM_MAX]) {
					mem[y][0][MEM_MAX] = x;
					mem[y][1][MEM_MAX] = z;
					memC[y][MEM_MAX] = color;
				}
			}
		}
	}
}
	
//! Determines color blend from given data
Color CVideoDriver::blendColor(Color color0, Color color1, int start, int end, int value)
{
	Color color = color1;
	if(start != end) {
		color.lerp(color0, (ABS(value-end)*100)/ABS(start-end));
	} else {
		color = color0;
	}
	return color;
}
