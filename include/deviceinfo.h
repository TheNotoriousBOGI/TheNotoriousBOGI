#ifndef DEVICEINFO_HEADER
#define DEVICEINFO_HEADER


#include "obis.h"

//device indentifiers (change for every device)
char device_ID[MSIZE] = "SUITE_019";                    // Change for every device

//firmware info
char firmwareVersion[MSIZE] = "2022-02-09";

//general device (ehz) information
char ServerID_string[MSIZE];
char ServerID_string_formatted[MSIZE];
char eHZ_Message[MSIZE] = "";                           // buffer for input data
char manufacturer[MSIZE] = "hager";                     // change 


#endif