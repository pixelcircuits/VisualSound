//-----------------------------------------------------------------------------------------
// Title:	Song Data Manager
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#include "CSongDataManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <stdlib.h>

//! Main constructor
CSongDataManager::CSongDataManager(const char* filePath) :
	visualizer(0), style(0), colorPrimary(155,155,155), colorSecondary(155,155,155), defaulterEnabled(false)
{
	this->filePath = filePath;
	if(filePath) {
		std::ifstream in(filePath);
		std::ostringstream ss;
		ss << in.rdbuf();
		fileData = ss.str();
		in.close();
	}
}

//! Destructor
CSongDataManager::~CSongDataManager()
{
}

//! Search for song data
bool CSongDataManager::findSongData(const char* artist, const char* album, const char* title)
{
	if(filePath && fileData.size() > 0) {
		char searchText[256];
		std::sprintf(searchText, "[%s][%s][%s]:", artist, album, title);
		const char* found = std::strstr(fileData.c_str(), searchText);
		if(found) {
			int i;
			char data[512];
			found += std::strlen(searchText);
			for(i=0; found[i] && found[i]!=13 && found[i]!=10 && i<(512-1); i++) data[i] = found[i];
			data[i] = 0;
			
			int cp[3];
			int cs[3];
			if(sscanf(data, "{visualizer:%d,style:%d,colorPrimary:[%d,%d,%d],colorSecondary:[%d,%d,%d]}", &visualizer, &style, &(cp[0]), &(cp[1]), &(cp[2]), &(cs[0]), &(cs[1]), &(cs[2]))==8) {
				colorPrimary = Color(cp[0],cp[1],cp[2]);
				colorSecondary = Color(cs[0],cs[1],cs[2]);
				return true;
			}
		} else if(defaulterEnabled) {
			unsigned long hash = 5381;
			hash = hashStr(artist, hash);
			hash = hashStr(album, hash);
			hash = hashStr(title, hash);
			
			visualizer = hashStr("visualizer", hash) % 100;
			style = hashStr("style", hash) % 100;
			
			const int numColors = 36;
			unsigned long color = hashStr("color", hash) % 36;
			if(color < (numColors/4)*1) {
				colorPrimary = Color::fromHSV(((color-(numColors/4)*0)*(360/(numColors/4)))%360,90,90);
				colorSecondary = Color::fromHSV((((color-(numColors/4)*0)*(360/(numColors/4))+40))%360,90,90);
			} else if(color < (numColors/4)*2) {
				colorPrimary = Color::fromHSV(((color-(numColors/4)*1)*(360/(numColors/4)))%360,90,90);
				colorSecondary = Color::fromHSV((((color-(numColors/4)*1)*(360/(numColors/4))+80))%360,90,90);
			} else if(color < (numColors/4)*3) {
				colorPrimary = Color::fromHSV(((color-(numColors/4)*2)*(360/(numColors/4)))%360,90,90);
				colorSecondary = Color::fromHSV((((color-(numColors/4)*2)*(360/(numColors/4))+120))%360,90,90);
			} else {
				colorPrimary = Color::fromHSV(((color-(numColors/4)*3)*(360/(numColors/4)))%360,90,90);
				colorSecondary = Color(0,0,0);
			}
			
			return true;
		}
	}
	return false;
}

//! Gets the song visualizer value
int CSongDataManager::getVisualizer()
{
	return visualizer;
}

//! Gets the song style value
int CSongDataManager::getStyle()
{
	return style;
}

//! Gets the song primary color
Color CSongDataManager::getColorPrimary()
{
	return colorPrimary;
}

//! Gets the song secondary color
Color CSongDataManager::getColorSecondary()
{
	
	return colorSecondary;
}

//! Saves the given song data
void CSongDataManager::saveSongData(const char* artist, const char* album, const char* title, int visualizer, int style, Color colorPrimary, Color colorSecondary)
{
	if(filePath) {
		const char* data = fileData.c_str();
		
		char songData[512];
		std::sprintf(songData, "[%s][%s][%s]:{visualizer:%d,style:%d,colorPrimary:[%d,%d,%d],colorSecondary:[%d,%d,%d]}", artist, album, title, visualizer, style, 
			colorPrimary.Red, colorPrimary.Green, colorPrimary.Blue, colorSecondary.Red, colorSecondary.Green, colorSecondary.Blue);
		
		char searchText[256];
		std::sprintf(searchText, "[%s][%s][%s]:", artist, album, title);
		const char* found = std::strstr(data, searchText);
		if(found) {
			int start = found-data;
			int len = 0;
			while(fileData[start+len] && fileData[start+len]!=13 && fileData[start+len]!=10) len++;
		
			fileData = fileData.substr(0,start) + songData + fileData.substr(start+len,fileData.size()-(start+len));
		} else {
			
			if(fileData.size() > 0) fileData = fileData + "\n";
			fileData = fileData + songData;
		}
		
		//remove trailing whitespace
		int end = fileData.size()-1;
		while(end > 0 && (fileData[end]==' ' || fileData[end]==13 || fileData[end]==10 || fileData[end]==9)) end--;
		fileData = fileData.substr(0, end+1);
		
		//write to file
		std::ofstream out(filePath);
		out << fileData;
		out.close();
	}
}

//! Enables the song data defaulter
void CSongDataManager::enableDefaulter(bool enabled)
{
	defaulterEnabled = enabled;
}

//! Generates hash from given string
unsigned long CSongDataManager::hashStr(const char *str, unsigned long hash)
{
	if(!str) return hash;
	while(*str) {
		unsigned long c = *str;
		hash = ((hash << 5) + hash) + c; // hash * 33 + c 
		str++;
	}
	return hash;
}
