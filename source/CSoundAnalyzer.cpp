//-----------------------------------------------------------------------------------------
// Title:	Sound Analyzer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CSoundAnalyzer.h"
#include "snd.h"
#include <complex>
#include <iostream>
#include <valarray>
 
typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

static const double PI = 3.141592653589793238460;
static void fft(CArray& x);

//! Main constructor
CSoundAnalyzer::CSoundAnalyzer() :
	sampFreq(SND_DEFAULT_SAMPLE_FREQUENCY), waveLPF(SND_DEFAULT_WAVE_LP_FILTER), waveTimeSmooth(SND_DEFAULT_WAVE_TIME_SMOOTH), 
	specSmoothPass(SND_DEFAULT_SPEC_SMOOTH_PASS), specTimeSmooth(SND_DEFAULT_SPEC_TIME_SMOOTH)
{
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE; i++) {
		waveLeft[i] = 0;
		waveRight[i] = 0;
		specLeft[i] = 0;
		specRight[i] = 0;
	}
	
	bassLeft = 0;
	midLeft = 0;
	trebLeft = 0;
	vuLeft = 0;
	bassRight = 0;
	midRight = 0;
	trebRight = 0;
	vuRight = 0;
}
	
//! Sets the sampling frequency
void CSoundAnalyzer::setSamplingFrequency(unsigned int sampFreq)
{
	this->sampFreq = sampFreq;
}

//! Sets the low pass filter value on wave data (1.0 = off)
void CSoundAnalyzer::setWaveLPF(float waveLPF)
{
	this->waveLPF = waveLPF;
}

//! Sets the time smooth value for wave data (1.0 = off)
void CSoundAnalyzer::setWaveTimeSmooth(float waveTimeSmooth)
{
	this->waveTimeSmooth = waveTimeSmooth;
}

//! Sets the smooth factor for spectrum data
void CSoundAnalyzer::setSpecSmoothPass(unsigned char specSmoothPass)
{
	this->specSmoothPass = specSmoothPass;
}

//! Sets the time smooth value for spectrum data (1.0 = off)
void CSoundAnalyzer::setSpecTimeSmooth(float specTimeSmooth)
{
	this->specTimeSmooth = specTimeSmooth;
}

//! Gets the left waveform samples
short CSoundAnalyzer::getWaveLeft(int index)
{
	if(index < 0) return waveLeft[0];
	if(index > (SND_BUFFER_SAMPLE_SIZE-1)) return waveLeft[SND_BUFFER_SAMPLE_SIZE-1];
	return waveLeft[index];
}

//! Gets the right waveform samples
short CSoundAnalyzer::getWaveRight(int index)
{
	if(index < 0) return waveRight[0];
	if(index > (SND_BUFFER_SAMPLE_SIZE-1)) return waveRight[SND_BUFFER_SAMPLE_SIZE-1];
	return waveRight[index];
}

//! Gets the left spectrum samples
short CSoundAnalyzer::getSpecLeft(int index)
{
	if(index < 0) return specLeft[0];
	if(index > (SND_BUFFER_SAMPLE_SIZE-1)) return specLeft[SND_BUFFER_SAMPLE_SIZE-1];
	return specLeft[index];
}

//! Gets the right spectrum samples
short CSoundAnalyzer::getSpecRight(int index)
{
	if(index < 0) return specRight[0];
	if(index > (SND_BUFFER_SAMPLE_SIZE-1)) return specRight[SND_BUFFER_SAMPLE_SIZE-1];
	return specRight[index];
}
	
//! Gets the left bass value
int CSoundAnalyzer::getBassLeft()
{
	return bassLeft;
}

//! Gets the right bass value
int CSoundAnalyzer::getBassRight()
{
	return bassRight;
}

//! Gets the left mid value
int CSoundAnalyzer::getMidLeft()
{
	return midLeft;
}

//! Gets the right mid value
int CSoundAnalyzer::getMidRight()
{
	return midRight;
}

//! Gets the left treble value
int CSoundAnalyzer::getTrebLeft()
{
	return trebLeft;
}

//! Gets the right treble value
int CSoundAnalyzer::getTrebRight()
{
	return trebRight;
}
	
//! Gets the left channel vloume
int CSoundAnalyzer::getVULeft()
{
	return vuLeft;
}

//! Gets the right channel vloume
int CSoundAnalyzer::getVURight()
{
	return vuRight;
}

//! Clears the video buffer to the specified color
void CSoundAnalyzer::refresh()
{
	snd_collectSamples(waveRaw, sampFreq, SND_BUFFER_SAMPLE_SIZE*2);

	//wave processing
	short wL = 0;
	short wR = 0;
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE; i++) {
		wL = wL*(1.0-waveLPF) + waveRaw[i*2 +0]*waveLPF;
		waveLeft[i] = waveLeft[i]*(1.0-waveTimeSmooth) + wL*waveTimeSmooth;
		wR = wR*(1.0-waveLPF) + waveRaw[i*2 +1]*waveLPF;
		waveRight[i] = waveRight[i]*(1.0-waveTimeSmooth) + wR*waveTimeSmooth;
	}

	//spectrum analysis - fft
	Complex complexDataLeft[SND_BUFFER_SAMPLE_SIZE];
	Complex complexDataRight[SND_BUFFER_SAMPLE_SIZE];
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE; i++) {
		double m = 0.5 * (1 - cos(2*PI*i/(SND_BUFFER_SAMPLE_SIZE-1)));//hann function window
		complexDataLeft[i] = std::complex<double>(m*(double)waveRaw[i*2 +0], 0.0);
		complexDataRight[i] = std::complex<double>(m*(double)waveRaw[i*2 +1], 0.0);
	}
    CArray dataArrayLeft(complexDataLeft, SND_BUFFER_SAMPLE_SIZE);
    CArray dataArrayRight(complexDataRight, SND_BUFFER_SAMPLE_SIZE);
    fft(dataArrayLeft);
    fft(dataArrayRight);
	
	//spectrum analysis - smoothing (data mirrors at the middle after fft)
	double smoothBufferL[2][(SND_BUFFER_SAMPLE_SIZE/2)];
	double smoothBufferR[2][(SND_BUFFER_SAMPLE_SIZE/2)];
	for(int i=0; i<(SND_BUFFER_SAMPLE_SIZE/2); i++) {
		smoothBufferL[0][i] = std::abs(dataArrayLeft[i])/20.0;
		if(smoothBufferL[0][i] > 32767) smoothBufferL[0][i] = 32767;
		smoothBufferR[0][i] = std::abs(dataArrayRight[i])/20.0;
		if(smoothBufferR[0][i] > 32767) smoothBufferR[0][i] = 32767;
	}
	for(int i=0; i<specSmoothPass; i++) {
		int from = i%2;
		int to = (i+1)%2;
		smoothBufferL[to][0] = (smoothBufferL[from][0]+smoothBufferL[from][1])/3.0;
		smoothBufferL[to][(SND_BUFFER_SAMPLE_SIZE/2)-1] = (smoothBufferL[from][(SND_BUFFER_SAMPLE_SIZE/2)-1]+smoothBufferL[from][(SND_BUFFER_SAMPLE_SIZE/2)-2])/3.0;
		for(int j=1; j<(SND_BUFFER_SAMPLE_SIZE/2)-1; j++) smoothBufferL[to][j] = (smoothBufferL[from][j-1]+smoothBufferL[from][j]+smoothBufferL[from][j+1])/3.0;
		smoothBufferR[to][0] = (smoothBufferR[from][0]+smoothBufferR[from][1])/3.0;
		smoothBufferR[to][(SND_BUFFER_SAMPLE_SIZE/2)-1] = (smoothBufferR[from][(SND_BUFFER_SAMPLE_SIZE/2)-1]+smoothBufferR[from][(SND_BUFFER_SAMPLE_SIZE/2)-2])/3.0;
		for(int j=1; j<(SND_BUFFER_SAMPLE_SIZE/2)-1; j++) smoothBufferR[to][j] = (smoothBufferR[from][j-1]+smoothBufferR[from][j]+smoothBufferR[from][j+1])/3.0;
	}
	int buff = specSmoothPass%2;
	for(int i=0; i<(SND_BUFFER_SAMPLE_SIZE/2); i++) {
		specLeft[i] = specLeft[i]*(1.0-specTimeSmooth) + smoothBufferL[buff][i]*specTimeSmooth;
		specRight[i] = specRight[i]*(1.0-specTimeSmooth) + smoothBufferR[buff][i]*specTimeSmooth;
		specLeft[(SND_BUFFER_SAMPLE_SIZE-1)-i] = specLeft[i];
		specRight[(SND_BUFFER_SAMPLE_SIZE-1)-i] = specRight[i];
	}
	
	//calculate volume from spec data (data mirrors at the middle after fft)
	float vuL = 0;
	float vuR = 0;
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE/2; i++) {
		vuL += (float)specLeft[i]/(float)(SND_BUFFER_SAMPLE_SIZE/2);
		vuR += (float)specRight[i]/(float)(SND_BUFFER_SAMPLE_SIZE/2);
	}
	vuLeft = vuL;
	vuRight = vuR;
	vuL = 0; vuR = 0;
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE/24; i++) {
		vuL += (float)specLeft[i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
		vuR += (float)specRight[i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
	}
	bassLeft = vuL;
	bassRight = vuR;
	vuL = 0; vuR = 0;
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE/24; i++) {
		vuL += (float)specLeft[((SND_BUFFER_SAMPLE_SIZE/2)/3) + i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
		vuR += (float)specRight[((SND_BUFFER_SAMPLE_SIZE/2)/3) + i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
	}
	midLeft = vuL;
	midRight = vuR;
	vuL = 0; vuR = 0;
	for(int i=0; i<SND_BUFFER_SAMPLE_SIZE/24; i++) {
		vuL += (float)specLeft[((SND_BUFFER_SAMPLE_SIZE/2)/3)*2 + i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
		vuR += (float)specRight[((SND_BUFFER_SAMPLE_SIZE/2)/3)*2 + i]/(float)(SND_BUFFER_SAMPLE_SIZE/24);
	}
	trebLeft = vuL;
	trebRight = vuR;
}

//FFT functions (https://rosettacode.org/wiki/Fast_Fourier_transform)
static void fft(CArray& x)
{
    const size_t N = x.size();
    if (N <= 1) return;
 
    // divide
    CArray even = x[std::slice(0, N/2, 2)];
    CArray  odd = x[std::slice(1, N/2, 2)];
 
    // conquer
    fft(even);
    fft(odd);
 
    // combine
    for (size_t k = 0; k < N/2; ++k)
    {
        Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
        x[k    ] = even[k] + t;
        x[k+N/2] = even[k] - t;
    }
}
