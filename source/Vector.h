//-----------------------------------------------------------------------------------------
// Title:	Vector Object
// Program: VisualizerUI
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>

//! Class that handles Vector math
class Vector
{
public:
	//! Default constructor
	Vector() : X(0), Y(0), Z(0) {}

	//! Main constructor
	Vector(int x, int y, int z) : X(x), Y(y), Z(z) {}

	//! Copy constructor
	Vector(const Vector& other) : X(other.X), Y(other.Y), Z(other.Z) {}

	//! Lerp (linear interpolation 0-100)
	void lerp(const Vector& other, unsigned char per) {
		X = X + ((other.X-X)*per)/100;
		Y = Y + ((other.Y-Y)*per)/100;
		Z = Z + ((other.Z-Z)*per)/100;
	}
	
	//! Rotate
	void rotate(float pitch, float roll, float yaw) {
		calcRotationMatrix(pitch, roll, yaw);

		float px = X; float py = Y; float pz = Z;
		X = rmartix[0][0]*px + rmartix[0][1]*py + rmartix[0][2]*pz;
		Y = rmartix[1][0]*px + rmartix[1][1]*py + rmartix[1][2]*pz;
		Z = rmartix[2][0]*px + rmartix[2][1]*py + rmartix[2][2]*pz;
	}
	
	//! Operators
	Vector operator-() const { return Vector(-X, -Y, -Z); }
	Vector& operator=(const Vector& other) { X = other.X; Y = other.Y; Z = other.Z; return *this; }
	Vector operator+(const Vector& other) const { return Vector(X + other.X, Y + other.Y, Z + other.Z); }
	Vector& operator+=(const Vector& other) { X+=other.X; Y+=other.Y; Z+=other.Z; return *this; }
	Vector operator-(const Vector& other) const { return Vector(X - other.X, Y - other.Y, Z - other.Z); }
	Vector& operator-=(const Vector& other) { X-=other.X; Y-=other.Y; Z-=other.Z; return *this; }
	Vector operator*(const Vector& other) const { return Vector(X * other.X, Y * other.Y, Z * other.Z); }
	Vector& operator*=(const Vector& other) { X*=other.X; Y*=other.Y; Z*=other.Z; return *this; }
	Vector operator*(const int v) const { return Vector(X * v, Y * v, Z * v); }
	Vector& operator*=(const int v) { X*=v; Y*=v; Z*=v; return *this; }
	Vector operator/(const Vector& other) const { return Vector(X / other.X, Y / other.Y, Z / other.Z); }
	Vector& operator/=(const Vector& other) { X/=other.X; Y/=other.Y; Z/=other.Z; return *this; }
	Vector operator/(const int v) const { return Vector(X / v, Y / v, Z / v); }
	Vector& operator/=(const int v) { X/=v; Y/=v; Z/=v; return *this; }
	bool operator==(const Vector& other) const { return X==other.X && Y==other.Y && Z==other.Z; }
	bool operator!=(const Vector& other) const { return !(X==other.X && Y==other.Y && Z==other.Z); }

	//! X coordinate
	int X;

	//! Y coordinate
	int Y;

	//! Z coordinate
	int Z;
	
private:
	//! Calculates the rotation matrix
	void calcRotationMatrix(float pitch, float roll, float yaw) {
		if(pitch == 0.0f && roll == 0.0f && rmatrix_yaw == 0.0f) {
			rmartix[0][0] = 1; rmartix[0][1] = 0; rmartix[0][2] = 0;
			rmartix[1][0] = 0; rmartix[1][1] = 1; rmartix[1][2] = 0;
			rmartix[2][0] = 0; rmartix[2][1] = 0; rmartix[2][2] = 1;
		} else if(pitch != rmatrix_pitch || roll != rmatrix_roll || yaw != rmatrix_yaw) {
			float cosa = cos(yaw);
			float sina = sin(yaw);
			float cosb = cos(pitch);
			float sinb = sin(pitch);
			float cosc = cos(roll);
			float sinc = sin(roll);

			rmartix[0][0] = cosa*cosb;   rmartix[0][1] = cosa*sinb*sinc - sina*cosc;   rmartix[0][2] = cosa*sinb*cosc + sina*sinc;
			rmartix[1][0] = sina*cosb;   rmartix[1][1] = sina*sinb*sinc + cosa*cosc;   rmartix[1][2] = sina*sinb*cosc - cosa*sinc;
			rmartix[2][0] = -sinb;       rmartix[2][1] = cosb*sinc;                    rmartix[2][2] = cosb*cosc;
		
			rmatrix_pitch = pitch; rmatrix_roll = roll; rmatrix_yaw = yaw;
		}
	}
	
	//! Rotation matrix data
	float rmatrix_pitch;
	float rmatrix_roll;
	float rmatrix_yaw;
	float rmartix[3][3];
};

#endif
