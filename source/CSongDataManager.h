//-----------------------------------------------------------------------------------------
// Title:	Song Data Manager
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef SONG_DATA_MANAGER_H
#define SONG_DATA_MANAGER_H

#include "Color.h"
#include <string>

//! Manages Settings Data
class CSongDataManager
{
public:
	//! Main Constructor
	CSongDataManager(const char* filePath);

	//! Destructor
	~CSongDataManager();
	
	//! Search for song data
	bool findSongData(const char* artist, const char* album, const char* title);
	
	//! Gets the song visualizer value
	int getVisualizer();
	
	//! Gets the song style value
	int getStyle();
	
	//! Gets the song primary color
	Color getColorPrimary();
	
	//! Gets the song secondary color
	Color getColorSecondary();
	
	//! Saves the given song data
	void saveSongData(const char* artist, const char* album, const char* title, int visualizer, int style, Color colorPrimary, Color colorSecondary);
	
private:
	std::string fileData;
	const char* filePath;
	int visualizer;
	int style;
	Color colorPrimary;
	Color colorSecondary;

	//Util functions
	void saveFile();
};

#endif
