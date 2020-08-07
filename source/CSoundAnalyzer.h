//-----------------------------------------------------------------------------------------
// Title:	Sound Analyzer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef SOUND_ANALYZER_H
#define SOUND_ANALYZER_H

#define SND_BUFFER_SAMPLE_SIZE 512

#define SND_DEFAULT_SAMPLE_FREQUENCY 6000
#define SND_DEFAULT_WAVE_LP_FILTER 1.0
#define SND_DEFAULT_WAVE_TIME_SMOOTH 1.0
#define SND_DEFAULT_SPEC_SMOOTH_PASS 0
#define SND_DEFAULT_SPEC_TIME_SMOOTH 1.0


//! Class that does all sound data processing
class CSoundAnalyzer
{
public:
	//! Main constructor
	CSoundAnalyzer();
	
	//! Sets the sampling frequency
	void setSamplingFrequency(unsigned int sampFreq);
	
	//! Sets the low pass filter value on wave data (1.0 = off)
	void setWaveLPF(float waveLPF);
	
	//! Sets the time smooth value for wave data (1.0 = off)
	void setWaveTimeSmooth(float waveTimeSmooth);
	
	//! Sets the smooth factor for spectrum data
	void setSpecSmoothPass(unsigned char specSmoothPass);
	
	//! Sets the time smooth value for spectrum data (1.0 = off)
	void setSpecTimeSmooth(float specTimeSmooth);
	
	//! Gets the left waveform samples
	short getWaveLeft(int index);
	
	//! Gets the right waveform samples
	short getWaveRight(int index);

	//! Gets the left spectrum samples
	short getSpecLeft(int index);
	
	//! Gets the right spectrum samples
	short getSpecRight(int index);
	
	//! Gets the left bass value
	int getBassLeft();
	
	//! Gets the right bass value
	int getBassRight();
	
	//! Gets the left mid value
	int getMidLeft();
	
	//! Gets the right mid value
	int getMidRight();
	
	//! Gets the left treble value
	int getTrebLeft();
	
	//! Gets the right treble value
	int getTrebRight();
	
	//! Gets the left channel volume
	int getVULeft();
	
	//! Gets the right channel volume
	int getVURight();

	//! Refreshes the sound data
	void refresh();

private:
	short waveRaw[SND_BUFFER_SAMPLE_SIZE*2];
	short waveLeft[SND_BUFFER_SAMPLE_SIZE];
	short waveRight[SND_BUFFER_SAMPLE_SIZE];
	short specLeft[SND_BUFFER_SAMPLE_SIZE];
	short specRight[SND_BUFFER_SAMPLE_SIZE];
	int bassLeft;
	int midLeft;
	int trebLeft;
	int vuLeft;
	int bassRight;
	int midRight;
	int trebRight;
	int vuRight;
	
	unsigned int sampFreq;
	float waveLPF;
	float waveTimeSmooth;
	unsigned char specSmoothPass;
	float specTimeSmooth;
};

#endif
