#ifndef DBS_H
#define DBS_H

// Setup and initialize the DBus utils
int dbs_init();

// Checks if the DBus utils are initialized
char dbs_isInit();

// Makes a simple method call with no arguments or reply
void dbs_simpleMethodCall(const char* objectPath, const char* busName, const char* interfaceName, const char* methodName);

// Makes a complex method call with arguments and expects a reply
void dbs_complexMethodCall(char* replyBuffer, int replyBufferSize, const char* objectPath, const char* busName, const char* interfaceName, const char* methodName, const char* arg0, const char* arg1, const char* arg2);

// Cleans up the DBus utils
int dbs_close();

#endif /* DBS_H */
