/*
 * This code based on
 * 
 * "untar" is an extremely simple tar extractor
 *
 * Written by Tim Kientzle, March 2009.
 *
 * Released into the public domain.
 *
 * Ported to Arduino library by Alexander Emelainov (a.m.emelianov@gmail.com), August 2017
 *  https://github.com/emelianov/untarArduino
 *
 */

/*
 * This demo just opens /test.tar from local ESP8266 SPIFFS storage and extraction all files to back local storage.
 *
 */

#include <FS.h>
#include <untar.h>

#define FILENAME "/test.tar"

Tar<FS> tar(&SPIFFS);			// Declare and initialize class passing pointer to SPIFFS as target filesystem

void setup() {
  Serial.begin(74880);
  SPIFFS.begin();			// Initialize SPIFFS
  File f = SPIFFS.open(FILENAME, "r");	// Open source file
  if (f) {
    tar.open((Stream*)&f);		// Pass source file as Stream to Tar object
    tar.dest("/");			// Set destination prefix to append to all files in archive. Should start with "/" for SPIFFS
    tar.extract();			// Process with extraction
  } else {
    Serial.println("Error open .tar file");
  }
}

void loop() {
  //Nothing
}
