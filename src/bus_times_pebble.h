#ifndef BUS_TIMES_PEBBLE_H
#define BUS_TIMES_PEBBLE_H

// The URL that will serve responses
#define URL "http://192.168.0.18:3000/watch"
#define AGENCY "Grand River Transit"

// To make multi-version compilation simple, these type of config defines
// should be put in an external file, such as config.h
#define ANDROID false
 
// If this app is being compiled for Android, use a unique UUID that uses the
// httpebble prefix, otherwise you'll have to use the common UUID
// that is defined in http.h
#if ANDROID
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x42, 0xD8, 0x14, 0xFC, 0x26, 0x12, 0x14 }
#else
#define MY_UUID HTTP_UUID
#endif

PBL_APP_INFO(MY_UUID, "GRT", "Keanu Lee", 1, 0,  RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_STANDARD_APP);

#endif
