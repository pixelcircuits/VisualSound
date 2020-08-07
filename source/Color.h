//-----------------------------------------------------------------------------------------
// Title:	Color Object
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef COLOR_H
#define COLOR_H


//! Class that handles color math
class Color
{
public:
	//! Default constructor
	Color() : Red(0), Green(0), Blue(0) {}

	//! Main constructor
	Color(unsigned char r, unsigned char g, unsigned char b) : Red(r), Green(g), Blue(b) {}

	//! Copy constructor
	Color(const Color& other) : Red(other.Red), Green(other.Green), Blue(other.Blue) {}

	//! Lerp (linear interpolation 0-100)
	void lerp(const Color& other, unsigned char per) {
		int rDiff = (int)other.Red-(int)Red;
		Red = (int)Red + (rDiff*(int)per)/100;
		int gDiff = (int)other.Green-(int)Green;
		Green = (int)Green + (gDiff*(int)per)/100;
		int bDiff = (int)other.Blue-(int)Blue;
		Blue = (int)Blue + (bDiff*(int)per)/100;
	}

	//! Operators
	Color& operator=(const Color& other) {
		Red = other.Red; Green = other.Green; Blue = other.Blue; return *this;
	}
	Color operator+(const Color& other) const {
		signed short r = Red + other.Red; if(r > 255) r = 255;
		signed short g = Green + other.Green; if(g > 255) g = 255;
		signed short b = Blue + other.Blue; if(b > 255) b = 255;
		return Color(r, g, b);
	}
	Color& operator+=(const Color& other) {
		signed short r = Red + other.Red; if(r > 255) r = 255;
		signed short g = Green + other.Green; if(g > 255) g = 255;
		signed short b = Blue + other.Blue; if(b > 255) b = 255;
		Red=r; Green=g; Blue=b; return *this;
	}
	Color operator-(const Color& other) const {
		signed short r = Red - other.Red; if(r < 0) r = 0;
		signed short g = Green - other.Green; if(g < 0) g = 0;
		signed short b = Blue - other.Blue; if(b < 0) b = 0;
		return Color(r, g, b);
	}
	Color& operator-=(const Color& other) {
		signed short r = Red - other.Red; if(r < 0) r = 0;
		signed short g = Green - other.Green; if(g < 0) g = 0;
		signed short b = Blue - other.Blue; if(b < 0) b = 0;
		Red=r; Green=g; Blue=b; return *this;
	}
	Color operator*(const Color& other) const {
		unsigned short r = Red * other.Red; if(r > 255) r = 255;
		unsigned short g = Green * other.Green; if(g > 255) g = 255;
		unsigned short b = Blue * other.Blue; if(b > 255) b = 255;
		return Color(r, g, b);
	}
	Color& operator*=(const Color& other) {
		unsigned short r = Red * other.Red; if(r > 255) r = 255;
		unsigned short g = Green * other.Green; if(g > 255) g = 255;
		unsigned short b = Blue * other.Blue; if(b > 255) b = 255;
		Red=r; Green=g; Blue=b; return *this;
	}
	Color operator*(const int v) const {
		int v2 = v;
		if(v2 > 255) v2 = 255;
		unsigned short r = Red * v2; if(r > 255) r = 255;
		unsigned short g = Green * v2; if(g > 255) g = 255;
		unsigned short b = Blue * v2; if(b > 255) b = 255;
		return Color(r, g, b);
	}
	Color& operator*=(const int v) {
		int v2 = v;
		if(v2 > 255) v2 = 255;
		unsigned short r = Red * v2; if(r > 255) r = 255;
		unsigned short g = Green * v2; if(g > 255) g = 255;
		unsigned short b = Blue * v2; if(b > 255) b = 255;
		Red=r; Green=g; Blue=b; return *this;
	}
	Color operator/(const Color& other) const {
		return Color(Red / other.Red, Green / other.Green, Blue / other.Blue);
	}
	Color& operator/=(const Color& other) {
		Red/=other.Red; Green/=other.Green; Blue/=other.Blue; return *this;
	}
	Color operator/(const int v) const {
		return Color(Red / v, Green / v, Blue / v);
	}
	Color& operator/=(const int v) {
		Red/=v; Green/=v; Blue/=v; return *this;
	}
	bool operator==(const Color& other) const {
		return Red==other.Red && Green==other.Green && Blue==other.Blue;
	}
	bool operator!=(const Color& other) const {
		return !(Red==other.Red && Green==other.Green && Blue==other.Blue);
	}
	
	//! Converts from HSV
	static Color fromHSV(unsigned int h, unsigned int s, unsigned int v) {
		if(h >= 360) h = 0;
		double hue = (double)h / 60.0;
		double sat = (double)s / 100.0;
		double val = (double)v / 100.0;
		
		long i = (long)hue;
		double ff = hue - i;
		double p = val * (1.0 - sat);
		double q = val * (1.0 - (sat * ff));
		double t = val * (1.0 - (sat * (1.0 - ff)));
		
		if(i==0) return Color(val*255, t*255, p*255);
		if(i==1) return Color(q*255, val*255, p*255);
		if(i==2) return Color(p*255, val*255, t*255);
		if(i==3) return Color(p*255, q*255, val*255);
		if(i==4) return Color(t*255,  p*255, val*255);
		if(i==5) return Color(val*255, p*255, q*255);
		return Color(0, 0, 0);
	}

	//! Red value
	unsigned char Red;
	
	//! Green value
	unsigned char Green;
	
	//! Blue value
	unsigned char Blue;
};

#endif
