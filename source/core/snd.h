#ifndef SND_H
#define SND_H

// Setup and initialize the Sound utils
int snd_init(const char* outputDevice);

// Checks if the Sound utils are initialized
char snd_isInit();

// Sets the input device
void snd_setInputDevice(const char* inputDevice);

// Sets the output device
void snd_setOutputDevice(const char* outputDevice);

// Gets the rate of the sound buffer
unsigned int snd_getBufferRate();

// Gets if the sound is running
char snd_getIsRunning();

// Fills the given buffer with data from the sound buffer
void snd_collectSamples(signed short* buffer, unsigned int sampleRate, unsigned int numSamples);

// Plays the given sound file
void snd_playFile(const char* filename);

// Sets the system volume [0-100]
void snd_setVolume(char volume);

// Gets the system volume [0-100]
unsigned char snd_getVolume();

// Cleans up the Sound utils
int snd_close();

#endif /* SND_H */
