
#include <FS.h>
#include <untar.h>

#define FILENAME "/test.tar"

Tar<FS> tar(&SPIFFS);

void setup() {
  Serial.begin(74880);
  SPIFFS.begin();
  tar.onFile
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
