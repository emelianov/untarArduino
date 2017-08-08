
#include <FS.h>
#include <untar.h>

#define FILENAME "/test.tar"

Tar<FS> tar(&SPIFFS);

void setup() {
  Serial.begin(74880);
  SPIFFS.begin();
  const char* dest = FILENAME;
	File f = SPIFFS.open(FILENAME, "r");
  if (f) {
    tar.f((Stream*)&f);
    tar.C("/");
	  tar.x(dest);
  } else {
    Serial.println("Error open .tar file");
  }
}

void loop() {

}