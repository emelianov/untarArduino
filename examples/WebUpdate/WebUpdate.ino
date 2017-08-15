/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <untar.h>
#include "StreamBuf.h"

const char* host = "esp8266-webupdate";
const char* ssid = "EW";
const char* password = "iMpress6264";

ESP8266WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
Tar<FS> tar(&SPIFFS);
#define BUF_LEN 1024
//int buf[BUF_LEN];
StreamBuf sb;
size_t total = 0;
bool tarFile(char* b) {
  return false;
}
void tarData(char* b, size_t s) {
  total += s;
  //Serial.println(total);
  //Serial.print(".");
  if(Update.write((uint8_t*)b, s) != s){
    Update.printError(Serial);
  }
}
void tarEof() {
  Serial.println("EOF");
  if(Update.end(true)){ //true to set the size to the current progress
    Serial.printf("Update Success: %u\nRebooting...\n", 0);
  } else {
    Update.printError(Serial);
  }
  Serial.setDebugOutput(false);
}
void setup(void){
  Serial.begin(74880);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  if(WiFi.waitForConnectResult() == WL_CONNECTED){
    SPIFFS.begin();
    tar.onFile(tarFile);
    tar.onData(tarData);
    tar.onEof(tarEof);
    tar.dest("/");
    MDNS.begin(host);
    server.on("/", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },[](){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Serial.println(maxSketchSpace);
          Update.printError(Serial);
        }
        tar.stream((Stream*)&sb);
      } else if(upload.status == UPLOAD_FILE_WRITE){
          //Serial.print("Block: ");
          //Serial.println(upload.currentSize);
          Serial.print(".");
          sb.open((uint8_t*)&upload.buf, upload.currentSize);
          tar.extract();
          //if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          //  Update.printError(Serial);
          //}
      }
      if(upload.status == UPLOAD_FILE_END){
        //if(Update.end(true)){ //true to set the size to the current progress
        //  Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        //} else {
          //Update.printError(Serial);
       //}
       // Serial.setDebugOutput(false);
      }
      yield();
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);
//Serial.println("NEW");
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}

void loop(void){
  server.handleClient();
  //Serial.print(".");
  delay(1000);
}