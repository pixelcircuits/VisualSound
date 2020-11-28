//-----------------------------------------------------------------------------------------
// Title:	Wave Music Visualizer
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CWaveVisualizer.h"
#include "../CVideoDriver.h"
#include "../CSoundAnalyzer.h"
#include <math.h>
#include <stdio.h>

#define STYLE_WAVE        0
#define STYLE_SPEC        1
#define STYLE_WAVE_FULL   2
#define STYLE_SPEC_FULL   3
#define NUM_STYLES        4

#define TRAIL_UPDATE_PERIOD 0.05

//! Main Constructor
CWaveVisualizer::CWaveVisualizer(CVideoDriver* vd, CSoundAnalyzer* sa) :
	videoDriver(vd), soundAnalyzer(sa), color1(0,0,0), color2(0,0,0), style(0)
{
}

//! Destructor
CWaveVisualizer::~CWaveVisualizer()
{
	clear();
}

//! Sets up the visualizer
void CWaveVisualizer::setup()
{
	soundAnalyzer->setSamplingFrequency(5000);
	soundAnalyzer->setWaveLPF(0.4f);
	soundAnalyzer->setWaveTimeSmooth(0.2f);
	soundAnalyzer->setSpecSmoothPass(64);
	soundAnalyzer->setSpecTimeSmooth(0.3f);
	
	//data storage
	int length = videoDriver->getDimension().X*2 + videoDriver->getDimension().Z*2;
	int maxDimension = videoDriver->getDimension().X;
	if(videoDriver->getDimension().Z > maxDimension) maxDimension = videoDriver->getDimension().Z;
	dataL = new short*[maxDimension];
	dataR = new short*[maxDimension];
	for(int i=0; i<maxDimension; i++) {
		dataL[i] = new short[length];
		dataR[i] = new short[length];
		for(int j=0; j<length; j++) {
			dataL[i][j] = 0;
			dataR[i][j] = 0;
		}
	}
	captureTime = 0;
	captureCount = 0;
}

//! Draws the visualizer
void CWaveVisualizer::draw(double elapsedTime)
{
	//compute basic values
	float intensity = (float)(soundAnalyzer->getVURight()+soundAnalyzer->getVULeft())/6000.0f;
	int videoOversample = videoDriver->getOversample();
	int videoDimX = videoDriver->getDimension().X;
	int videoDimY = videoDriver->getDimension().Y;
	int videoDimZ = videoDriver->getDimension().Z;
	
	//check for data capture
	int length = videoDimX*2 + videoDimZ*2;
	int maxDimension = videoDimX;
	if(videoDimZ > maxDimension) maxDimension = videoDimZ;
	captureTime+=elapsedTime;
	if(captureTime > TRAIL_UPDATE_PERIOD/(1.0+intensity)) {
		captureTime = 0;
		captureCount++;
		short* dataLEnd = dataL[maxDimension-1];
		short* dataREnd = dataR[maxDimension-1];
		for(int i=maxDimension-1; i>0; i--) {
			dataL[i] = dataL[i-1];
			dataR[i] = dataR[i-1];
		}
		dataL[0] = dataLEnd;
		dataR[0] = dataREnd;
		
		if(style == STYLE_WAVE ||style == STYLE_WAVE_FULL) {
			int waveStart = 20;
			float waveLength = 0.08f*((float)SND_BUFFER_SAMPLE_SIZE/(float)length);
			for(int i=0; i<length; i++) {
				dataL[0][i] = soundAnalyzer->getWaveLeft((int)(waveStart + i*waveLength));
				dataR[0][i] = soundAnalyzer->getWaveRight((int)(waveStart + i*waveLength));
			}
		} else if(style == STYLE_SPEC ||style == STYLE_SPEC_FULL) {
			float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)length);
			for(int i=0; i<length; i++) {
				dataL[0][i] = soundAnalyzer->getSpecLeft((int)(i*specLength));
				dataR[0][i] = soundAnalyzer->getSpecRight((int)(i*specLength));
			}
		}
	}
	
	//clear buffer
	videoDriver->clearVideoBuffer(Color(0,0,0));
	
	//common properties accross all visualizers
	Color highlightColor = Color(250,250,250);
	
	//wave trail style
	if(style == STYLE_WAVE) {
		const int waveStart = 20;
		float waveLength = 0.08f*((float)SND_BUFFER_SAMPLE_SIZE/(float)videoDimX);
		float waveAmplitude = 0.12f*((float)videoDimY/1000.0f);
		for(int i=0; i<videoDimX; i++) {
			int i2 = i+1;
			int val = soundAnalyzer->getWaveLeft((int)(waveStart + i*waveLength))*waveAmplitude;
			int val2 = soundAnalyzer->getWaveLeft((int)(waveStart + i2*waveLength))*waveAmplitude;
			for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
				videoDriver->drawLine(Vector(i,(videoDimY/2)+val,0)+Vector(0,-videoOversample+y,z), highlightColor, Vector(i2,(videoDimY/2)+val2,0)+Vector(0,-videoOversample+y,z), highlightColor);
			}
			for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
				videoDriver->drawPoint((Vector(i,(videoDimY/2)+val,0)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), highlightColor);
			}
		}
		for(int j=videoOversample; j<videoDimZ; j++) {
			for(int i=0; i<videoDimX; i++) {
				int i2 = i+1;
				int val = dataL[j-videoOversample][i]*waveAmplitude;
				int val2 = dataR[j-videoOversample][i2]*waveAmplitude;
				
				Color c = color1;
				int colorLen = videoDimZ/2;
				int ci = ((videoDimZ-j)+captureCount)%(colorLen*2);
				if(ci < colorLen) c.lerp(color2, (ci*100)/colorLen);
				else c.lerp(color2, 100-(((ci-colorLen)*100)/colorLen));
				for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawLine(Vector(i,(videoDimY/2)+val,j)+Vector(0,-videoOversample+y,z), c, Vector(i2,(videoDimY/2)+val2,j)+Vector(0,-videoOversample+y,z), c);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawPoint((Vector(i,(videoDimY/2)+val,j)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), c);
				}
			}
		}
	}
	
	//spec trail style
	if(style == STYLE_SPEC) {
		float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)videoDimX);
		float specAmplitude = 0.11f*((float)videoDimY/1000.0f);
		for(int i=0; i<videoDimX; i++) {
			int valL = soundAnalyzer->getSpecLeft((int)(i*specLength))*specAmplitude;
			int valR = soundAnalyzer->getSpecRight((int)(i*specLength))*specAmplitude;
			for(int z=0; z<videoOversample; z++) {
				videoDriver->drawLine(Vector(i,(videoDimY/2)+videoOversample/2+valL,0)+Vector(0,-videoOversample,z), highlightColor/3, Vector(i,(videoDimY/2)-valR,0)+Vector(0,-videoOversample,z), highlightColor/3);
			}
			for(int z=0; z<videoOversample; z++) for(int y=0; y<videoOversample; y++) {
				videoDriver->drawPoint(Vector(i,(videoDimY/2)+valL,0)+Vector(0,y-videoOversample,z), highlightColor);
				videoDriver->drawPoint(Vector(i,(videoDimY/2)-valR,0)+Vector(0,y-videoOversample,z), highlightColor);
			}
		}
		for(int z=videoOversample; z<videoDimZ; z++) {
			for(int x=0; x<videoDimX; x++) {
				int valL = dataL[z-videoOversample][x*4]*specAmplitude;
				int valR = dataR[z-videoOversample][x*4]*specAmplitude;
				
				Color c = color1;
				int colorLen = videoDimZ/2;
				int ci = ((videoDimZ-z)+captureCount)%(colorLen*2);
				if(ci < colorLen) c.lerp(color2, (ci*100)/colorLen);
				else c.lerp(color2, 100-(((ci-colorLen)*100)/colorLen));
				videoDriver->drawLine(Vector(x,(videoDimY/2)+videoOversample/2+valL,z)+Vector(0,-videoOversample,0), c, Vector(x,(videoDimY/2)-valR,z)+Vector(0,-videoOversample,0), c);
			}
		}
	}
	
	//spec surround style
	if(style == STYLE_SPEC_FULL) {
		float specLength = 1.0f*((float)(SND_BUFFER_SAMPLE_SIZE/2)/(float)(videoDimZ+videoDimX+videoDimZ+videoDimX));
		float specAmplitude = 0.11f*((float)videoDimY/1000.0f);
		double angleOffset = ((double)captureCount)*M_PI*0.035;
		int midNumL = 0;
		midNumL += dataL[(videoDimX/2)-videoOversample*2][videoDimZ/2]*specAmplitude;
		midNumL += dataL[(videoDimZ/2)-videoOversample*2][videoDimZ+(videoDimX/2)]*specAmplitude;
		midNumL += dataL[(videoDimX/2)-videoOversample*2][videoDimZ+videoDimX+(videoDimZ/2)]*specAmplitude;
		midNumL += dataL[(videoDimZ/2)-videoOversample*2][videoDimZ+videoDimX+videoDimZ+(videoDimX/2)]*specAmplitude;
		midNumL /= 6;
		int midNumR = 0;
		midNumR += dataR[(videoDimX/2)-videoOversample][videoDimZ/2]*specAmplitude;
		midNumR += dataR[(videoDimZ/2)-videoOversample][videoDimZ+(videoDimX/2)]*specAmplitude;
		midNumR += dataR[(videoDimX/2)-videoOversample][videoDimZ+videoDimX+(videoDimZ/2)]*specAmplitude;
		midNumR += dataR[(videoDimZ/2)-videoOversample][videoDimZ+videoDimX+videoDimZ+(videoDimX/2)]*specAmplitude;
		midNumR /= 6;
		
		for(int i=0; i<(videoDimZ+videoDimX+videoDimZ+videoDimX); i++) {
			int valL = soundAnalyzer->getSpecLeft((int)(i*specLength))*specAmplitude;
			int valR = soundAnalyzer->getSpecRight((int)(i*specLength))*specAmplitude;
			if(i<videoDimZ) {
				int j = (videoDimZ-1)-i;
				for(int x=0; x<videoOversample; x++) {
					videoDriver->drawLine(Vector(0,(videoDimY/2)+videoOversample/2+valL,j)+Vector(x,-videoOversample,0), highlightColor/3, Vector(0,((videoDimY/2))-valR,j)+Vector(x,-videoOversample,0), highlightColor/3);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) {
					videoDriver->drawPoint(Vector(0,(videoDimY/2)+valL,j)+Vector(x,y-videoOversample,0), highlightColor);
					videoDriver->drawPoint(Vector(0,(videoDimY/2)-valR,j)+Vector(x,y-videoOversample,0), highlightColor);
				}
				for(int x=videoOversample; x<videoDimX/2; x++) {
					valL = dataL[x-videoOversample][i]*specAmplitude;
					valR = dataR[x-videoOversample][i]*specAmplitude;
					valL = (valL*((videoDimX/2)-x) + midNumL*x)/(videoDimX/2);
					valR = (valR*((videoDimX/2)-x) + midNumR*x)/(videoDimX/2);
					if(j >= x && j < (videoDimX-x)) {
						double t = calcAngleRatio(x-(videoDimX/2), j-(videoDimZ/2), 1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						videoDriver->drawLine(Vector(x,(videoDimY/2)+videoOversample/2+valL,j)+Vector(0,-videoOversample,0), cs, Vector(x,((videoDimY/2))-valR,j)+Vector(0,-videoOversample,0), cs);
					}
				}
			} else if(i<videoDimZ+videoDimX) {
				int j = i-(videoDimZ);
				for(int z=0; z<videoOversample; z++) {
					videoDriver->drawLine(Vector(j,(videoDimY/2)+videoOversample/2+valL,0)+Vector(0,-videoOversample,z), highlightColor/3, Vector(j,((videoDimY/2))-valR,0)+Vector(0,-videoOversample,z), highlightColor/3);
				}
				for(int z=0; z<videoOversample; z++) for(int y=0; y<videoOversample; y++) {
					videoDriver->drawPoint(Vector(j,(videoDimY/2)+valL,0)+Vector(0,y-videoOversample,z), highlightColor);
					videoDriver->drawPoint(Vector(j,(videoDimY/2)-valR,0)+Vector(0,y-videoOversample,z), highlightColor);
				}
				for(int z=videoOversample; z<videoDimZ/2; z++) {
					valL = dataL[z-videoOversample][i]*specAmplitude;
					valR = dataR[z-videoOversample][i]*specAmplitude;
					valL = (valL*((videoDimZ/2)-z) + midNumL*z)/(videoDimZ/2);
					valR = (valR*((videoDimZ/2)-z) + midNumR*z)/(videoDimZ/2);
					if(j >= z && j < (videoDimZ-z)) {
						double t = calcAngleRatio(j-(videoDimX/2), z-(videoDimZ/2), 1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						videoDriver->drawLine(Vector(j,(videoDimY/2)+videoOversample/2+valL,z)+Vector(0,-videoOversample,0), cs, Vector(j,((videoDimY/2))-valR,z)+Vector(0,-videoOversample,0), cs);
					}
				}
			} else if(i<videoDimZ+videoDimX+videoDimZ) {
				int j = i-(videoDimZ+videoDimX);
				for(int x=0; x<videoOversample; x++) {
					videoDriver->drawLine(Vector((videoDimX-1),(videoDimY/2)+videoOversample/2+valL,j)+Vector(-x,-videoOversample,0), highlightColor/3, Vector((videoDimX-1),((videoDimY/2))-valR,j)+Vector(-x,-videoOversample,0), highlightColor/3);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) {
					videoDriver->drawPoint(Vector((videoDimX-1),(videoDimY/2)+valL,j)+Vector(-x,y-videoOversample,0), highlightColor);
					videoDriver->drawPoint(Vector((videoDimX-1),(videoDimY/2)-valR,j)+Vector(-x,y-videoOversample,0), highlightColor);
				}
				for(int x=videoOversample; x<videoDimX/2; x++) {
					valL = dataL[x-videoOversample][i]*specAmplitude;
					valR = dataR[x-videoOversample][i]*specAmplitude;
					valL = (valL*((videoDimX/2)-x) + midNumL*x)/(videoDimX/2);
					valR = (valR*((videoDimX/2)-x) + midNumR*x)/(videoDimX/2);
					if(j >= x && j < (videoDimX-x)) {
						double t = calcAngleRatio(((videoDimX-1)-x)-(videoDimX/2), j-(videoDimZ/2), -1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						videoDriver->drawLine(Vector((videoDimX-1)-x,(videoDimY/2)+videoOversample/2+valL,j)+Vector(0,-videoOversample,0), cs, Vector((videoDimX-1)-x,((videoDimY/2))-valR,j)+Vector(0,-videoOversample,0), cs);
					}
				}
			} else if(i<videoDimZ+videoDimX+videoDimZ+videoDimX) {
				int j = (videoDimX-1)-(i-(videoDimZ+videoDimX+videoDimZ));
				for(int z=0; z<videoOversample; z++) {
					videoDriver->drawLine(Vector(j,(videoDimY/2)+videoOversample/2+valL,(videoDimZ-1))+Vector(0,-videoOversample,-z), highlightColor/3, Vector(j,((videoDimY/2))-valR,(videoDimZ-1))+Vector(0,-videoOversample,-z), highlightColor/3);
				}
				for(int z=0; z<videoOversample; z++) for(int y=0; y<videoOversample; y++) {
					videoDriver->drawPoint(Vector(j,(videoDimY/2)+valL,(videoDimZ-1))+Vector(0,y-videoOversample,-z), highlightColor);
					videoDriver->drawPoint(Vector(j,(videoDimY/2)-valR,(videoDimZ-1))+Vector(0,y-videoOversample,-z), highlightColor);
				}
				for(int z=videoOversample; z<videoDimZ/2; z++) {
					valL = dataL[z-videoOversample][i]*specAmplitude;
					valR = dataR[z-videoOversample][i]*specAmplitude;
					valL = (valL*((videoDimZ/2)-z) + midNumL*z)/(videoDimZ/2);
					valR = (valR*((videoDimZ/2)-z) + midNumR*z)/(videoDimZ/2);
					if(j >= z && j < (videoDimZ-z)) {
						double t = calcAngleRatio(j-(videoDimX/2), ((videoDimZ-1)-z)-(videoDimZ/2), -1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						videoDriver->drawLine(Vector(j,(videoDimY/2)+videoOversample/2+valL,(videoDimZ-1)-z)+Vector(0,-videoOversample,0), cs, Vector(j,((videoDimY/2))-valR,(videoDimZ-1)-z)+Vector(0,-videoOversample,0), cs);
					}
				}
			}
		}
	}
	
	//wave surround style
	if(style == STYLE_WAVE_FULL) {
		const int waveStart = 20;
		float waveLength = 0.08f*((float)SND_BUFFER_SAMPLE_SIZE/(float)videoDimX);
		float waveAmplitude = 0.10f*((float)videoDimY/1000.0f);
		double angleOffset = ((double)captureCount)*M_PI*0.035;
		int midDim = 2;
		for(int i=0; i<(videoDimZ+videoDimX+videoDimZ+videoDimX)-1; i++) {
			int i2 = i+1;
			int val = soundAnalyzer->getWaveLeft((int)(waveStart + i*waveLength))*waveAmplitude;
			int val2 = soundAnalyzer->getWaveRight((int)(waveStart + i2*waveLength))*waveAmplitude;
			if(i<videoDimZ) {
				int j = (videoDimZ-1)-i;
				int j2 = j+1;
				for(int y=0; y<videoOversample; y++) for(int x=0; x<videoOversample; x++) {
					videoDriver->drawLine(Vector(0,(videoDimY/2)+val,j)+Vector(x,-videoOversample+y,0), highlightColor, Vector(0,(videoDimY/2)+val2,j2)+Vector(x,-videoOversample+y,0), highlightColor);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawPoint((Vector(0,(videoDimY/2)+val,j)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), highlightColor);
				}
				for(int l=videoOversample; l<videoDimX/2; l++) {
					val = dataL[l-videoOversample][i]*waveAmplitude;
					val2 = dataL[l-videoOversample][i2]*waveAmplitude;
					val = (val*((videoDimX/2)-l) + (val*l)/midDim)/(videoDimX/2);
					val2 = (val2*((videoDimX/2)-l) + (val2*l)/midDim)/(videoDimX/2);
					if(j >= l && j < (videoDimX-l)) {
						double t = calcAngleRatio(l-(videoDimX/2), j-(videoDimZ/2), 1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						for(int y=0; y<videoOversample; y++) for(int x=0; x<videoOversample; x++) {
							videoDriver->drawLine(Vector(l,(videoDimY/2)+val,j)+Vector(x,-videoOversample+y,0), cs, Vector(l,(videoDimY/2)+val2,j2)+Vector(x,-videoOversample+y,0), cs);
						}
						for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawPoint((Vector(l,(videoDimY/2)+val,j)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), cs);
						}
					}
				}
			} else if(i<videoDimZ+videoDimX) {
				int j = i-(videoDimZ);
				int j2 = j+1;
				for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawLine(Vector(j,(videoDimY/2)+val,0)+Vector(0,-videoOversample+y,z), highlightColor, Vector(j2,(videoDimY/2)+val2,0)+Vector(0,-videoOversample+y,z), highlightColor);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawPoint((Vector(j,(videoDimY/2)+val,0)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), highlightColor);
				}
				for(int l=videoOversample; l<videoDimZ/2; l++) {
					val = dataL[l-videoOversample][i]*waveAmplitude;
					val2 = dataL[l-videoOversample][i2]*waveAmplitude;
					val = (val*((videoDimZ/2)-l) + (val*l)/midDim)/(videoDimZ/2);
					val2 = (val2*((videoDimZ/2)-l) + (val2*l)/midDim)/(videoDimZ/2);
					if(j >= l && j < (videoDimZ-l)) {
						double t = calcAngleRatio(j-(videoDimX/2), l-(videoDimZ/2), 1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawLine(Vector(j,(videoDimY/2)+val,l)+Vector(0,-videoOversample+y,z), cs, Vector(j2,(videoDimY/2)+val2,l)+Vector(0,-videoOversample+y,z), cs);
						}
						for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawPoint((Vector(j,(videoDimY/2)+val,l)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), cs);
						}
					}
				}
			} else if(i<videoDimZ+videoDimX+videoDimZ) {
				int j = i-(videoDimZ+videoDimX);
				int j2 = j+1;
				for(int y=0; y<videoOversample; y++) for(int x=0; x<videoOversample; x++) {
					videoDriver->drawLine(Vector((videoDimX-videoOversample),(videoDimY/2)+val,j)+Vector(x,-videoOversample+y,0), highlightColor, Vector((videoDimX-videoOversample),(videoDimY/2)+val2,j2)+Vector(x,-videoOversample+y,0), highlightColor);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawPoint((Vector((videoDimX-videoOversample),(videoDimY/2)+val,j)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), highlightColor);
				}
				for(int l=videoOversample; l<videoDimX/2; l++) {
					val = dataL[l-videoOversample][i]*waveAmplitude;
					val2 = dataL[l-videoOversample][i2]*waveAmplitude;
					val = (val*((videoDimX/2)-l) + (val*l)/midDim)/(videoDimX/2);
					val2 = (val2*((videoDimX/2)-l) + (val2*l)/midDim)/(videoDimX/2);
					if(j >= l && j < (videoDimX-l)) {
						double t = calcAngleRatio(((videoDimX-1)-l)-(videoDimX/2), j-(videoDimZ/2), -1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						for(int y=0; y<videoOversample; y++) for(int x=0; x<videoOversample; x++) {
							videoDriver->drawLine(Vector((videoDimX-videoOversample)-l,(videoDimY/2)+val,j)+Vector(x,-videoOversample+y,0), cs, Vector((videoDimX-videoOversample)-l,(videoDimY/2)+val2,j2)+Vector(x,-videoOversample+y,0), cs);
						}
						for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawPoint((Vector((videoDimX-videoOversample)-l,(videoDimY/2)+val,j)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), cs);
						}
					}
				}
			} else if(i<videoDimZ+videoDimX+videoDimZ+videoDimX) {
				int j = (videoDimX-1)-(i-(videoDimZ+videoDimX+videoDimZ));
				int j2 = j+1;
				for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawLine(Vector(j,(videoDimY/2)+val,(videoDimZ-videoOversample))+Vector(0,-videoOversample+y,z), highlightColor, Vector(j2,(videoDimY/2)+val2,(videoDimZ-videoOversample))+Vector(0,-videoOversample+y,z), highlightColor);
				}
				for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
					videoDriver->drawPoint((Vector(j,(videoDimY/2)+val,(videoDimZ-videoOversample))/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), highlightColor);
				}
				for(int l=videoOversample; l<videoDimZ/2; l++) {
					val = dataL[l-videoOversample][i]*waveAmplitude;
					val2 = dataL[l-videoOversample][i2]*waveAmplitude;
					val = (val*((videoDimZ/2)-l) + (val*l)/midDim)/(videoDimZ/2);
					val2 = (val2*((videoDimZ/2)-l) + (val2*l)/midDim)/(videoDimZ/2);
					if(j >= l && j < (videoDimZ-l)) {
						double t = calcAngleRatio(j-(videoDimX/2), ((videoDimZ-1)-l)-(videoDimZ/2), -1, angleOffset)*2.0;
						Color cs = color1;
						if(t < 1) cs.lerp(color2, t*100);
						else cs.lerp(color2, (2.0-t)*100);
						for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawLine(Vector(j,(videoDimY/2)+val,(videoDimZ-videoOversample)-l)+Vector(0,-videoOversample+y,z), cs, Vector(j2,(videoDimY/2)+val2,(videoDimZ-videoOversample)-l)+Vector(0,-videoOversample+y,z), cs);
						}
						for(int x=0; x<videoOversample; x++) for(int y=0; y<videoOversample; y++) for(int z=0; z<videoOversample; z++) {
							videoDriver->drawPoint((Vector(j,(videoDimY/2)+val,(videoDimZ-videoOversample)-l)/videoOversample)*videoOversample+Vector(x,-videoOversample+y,z), cs);
						}
					}
				}
			}
		}
	}
}

//! Sets the visualizer colors
void CWaveVisualizer::setColors(Color color1, Color color2)
{
	this->color1 = color1;
	this->color2 = color2;
}

//! Gets the visualizer colors
Color CWaveVisualizer::getColor(int num)
{
	if(num==0) return color1;
	if(num==1) return color2;
	return Color(0,0,0);
}

//! Sets the visualizer style
void CWaveVisualizer::setStyle(int style)
{
	while(style >= NUM_STYLES) style -= NUM_STYLES;
	while(style < 0) style += NUM_STYLES;
	if(this->style != style) {
		this->style = style;
		
		int length = videoDriver->getDimension().X*2 + videoDriver->getDimension().Z*2;
		int maxDimension = videoDriver->getDimension().X;
		if(videoDriver->getDimension().Z > maxDimension) maxDimension = videoDriver->getDimension().Z;
		for(int i=0; i<maxDimension; i++) {
			for(int j=0; j<length; j++) {
				dataL[i][j] = 0;
				dataR[i][j] = 0;
			}
		}
	}
}

//! Gets the visualizer style
int CWaveVisualizer::getStyle()
{
	return style;
}

//! Clears all resources of the visualizer
void CWaveVisualizer::clear()
{
	//free data
	int maxDimension = videoDriver->getDimension().X;
	if(videoDriver->getDimension().Z > maxDimension) maxDimension = videoDriver->getDimension().Z;
	for(int i=0; i<maxDimension; i++) {
		delete[] dataL[i];
		delete[] dataR[i];
	}
	delete[] dataL;
	delete[] dataR;
}

//! Calculates angle from 0 to 1
double CWaveVisualizer::calcAngleRatio(int x, int y, double zm, double offset)
{
	double t = zm*(M_PI/2.0f);
	if(y != 0) t = atan((double)x/(double)y);
	t += (M_PI/2.0f) + offset;
	while(t > M_PI) t -= M_PI;
	return (t/M_PI);
}
