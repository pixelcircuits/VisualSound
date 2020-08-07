#ifndef PAIR_H
#define PAIR_H

#define PAIR_MEDIA_STATE_NOT_CONNECTED 0
#define PAIR_MEDIA_STATE_PLAYING 1
#define PAIR_MEDIA_STATE_STOPPED 2
#define PAIR_MEDIA_STATE_PAUSED 3
#define PAIR_MEDIA_STATE_FORWARD_SEEK 4
#define PAIR_MEDIA_STATE_REVERSE_SEEK 5
#define PAIR_MEDIA_STATE_ERROR 6
#define PAIR_MEDIA_STATE_UNKNOWN 7

// Setup and initialize the BT Pairing utils
int pair_init();

// Checks if the BT Pairing utils are initialized
char pair_isInit();

// Resets to the pairing state
void pair_disconnectDevice();

// Sends the play command to the connected media device
void pair_mediaPlay();

// Sends the pause command to the connected media device
void pair_mediaPause();

// Sends the next command to the connected media device
void pair_mediaNext();

// Sends the previous command to the connected media device
void pair_mediaPrevious();

// Gets the state of the connected media device
int pair_mediaGetState();

// Gets the track position of the connected media device (ms)
int pair_mediaGetTrackPosition();

// Gets the track duration of the connected media device (ms)
int pair_mediaGetTrackDuration();

// Gets the track title of the connected media device
const char* pair_mediaGetTrackTitle();

// Gets the track album of the connected media device
const char* pair_mediaGetTrackAlbum();

// Gets the track artist of the connected media device
const char* pair_mediaGetTrackArtist();

// Cleans up the BT Pairing utils
int pair_close();

#endif /* PAIR_H */
