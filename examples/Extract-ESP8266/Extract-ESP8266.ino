
#include <FS.h>
#include <untar.h>

#define FILENAME "/test.tar"

Tar<FS> tar(&SPIFFS);

void setup() {
  const char* dest = "/";
	File f = SPIFFS.open(FILENAME, "r");
  tar.f((Stream*)&f);
	tar.x(dest);
}

void loop() {

}

