#include <Arduino.h>
#include <noveltyDetection.h>

void setup() {
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Hit any key to start reading from SPIFFS...");
  while (!Serial.available());
  SPIFFS.begin();
  EEPROM.begin(1024);
  Serial.println(SVM_readModelFromSPIFFS(SVM_MODEL_FILENAME, SVM_SCALE_PARAMETERS_FILENAME));
  Serial.println("done");
}

void loop() {
}
