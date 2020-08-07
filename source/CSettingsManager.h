//-----------------------------------------------------------------------------------------
// Title:	Settings Manager
// Program: VisualSound
// Authors: Stephen Monn
//-----------------------------------------------------------------------------------------
#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H
#include <string>

//! Manages Settings Data
class CSettingsManager
{
public:
	//! Main Constructor
	CSettingsManager(const char* filePath);

	//! Destructor
	~CSettingsManager();
	
	//! Gets the string value for the given property
	void getPropertyString(const char* property, const char* defVal, char* out, int max);

	//! Gets the integer value for the given property
	signed int getPropertyInteger(const char* property, signed int defVal);
	
	//! Sets the string value for the given property
	void setPropertyString(const char* property, const char* value);

	//! Sets the integer value for the given property
	void setPropertyInteger(const char* property, signed int value);
	
private:
	std::string fileData;
	const char* filePath;

	//Util functions
	void saveFile();
};

#endif
