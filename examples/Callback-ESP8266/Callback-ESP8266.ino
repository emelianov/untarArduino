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
 * Callback demo. Skips all files except 'data/create.txt' and print extraction process messages from callback not from library main code.
 *
 */

#include <FS.h>
#define TAR_CALLBACK
#define TAR_SILENT
#include <untar.h>

#define PIN D4

#define FILENAME "/test.tar"
#define EXTRACT "data/create.txt"

Tar<FS> tar(&SPIFFS);
bool write = false;
char* filename = EXTRACT;

bool printFile(char* name) {
	Serial.print(name);
	if (strcmp(name, filename) == 0) {
		Serial.println();
		write = true;
		return true;
	}
	Serial.println(" -  SKIP");
	return false;
}
void blinkWrite(char* data, size_t s) {
	if (!write) return;
	digitalWrite(PIN, !digitalRead(PIN));	
}

void eof() {
  digitalWrite(PIN, HIGH);  
	write = false;
}

void setup() {
  Serial.begin(74880);
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  SPIFFS.begin();
  tar.onFile(printFile);
  tar.onData(blinkWrite);
  tar.onEof(eof);
  File f = SPIFFS.open(FILENAME, "r");
  if (f) {
    tar.stream((Stream*)&f);
    tar.dest("/");
	  tar.extract();
  } else {
    Serial.println("Error open .tar file");
  }
}

void loop() {

}
