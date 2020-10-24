#ifndef INP_H
#define INP_H

#define INP_BTN_UP        0
#define INP_BTN_DOWN      1
#define INP_BTN_LEFT      2
#define INP_BTN_RIGHT     3
#define INP_BTN_HOME      4
#define INP_BTN_MENU      5
#define INP_BTN_BACK      6
#define INP_BTN_OK        7
#define INP_BTN_VOLUP     8
#define INP_BTN_VOLDOWN   9
#define INP_BTN_PWR       10
#define INP_BTN_TOTAL     11

#define INP_TYPE_UNKNOWN 0
#define INP_TYPE_KEYBOARD 1
#define INP_TYPE_JOYSTICK 2

// Setup and initialize the Controller interface
int inp_init();

// Checks if the Controller interface is initialized
char inp_isInit();

// Updates the button holding state
void inp_updateButtonState();

// Gets the holding state of the given button
unsigned char inp_getButtonState(char button);

// Sets the input device code for the given button
void inp_setButtonCode(char button, int code);

// Sets if the button should never be considered off
void inp_setButtonHold(char button, char hold);

// Closes the Controller interface
int inp_close();


#endif /* INP_H */
