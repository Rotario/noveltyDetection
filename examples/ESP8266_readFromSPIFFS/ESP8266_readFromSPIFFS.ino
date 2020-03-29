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
  Serial.println("Checking model works...");
  float sensor[] = {3.302e-05,-0.00185078,0.01562887,0.54396915,-11.1016785,2197.46678555};
  float ret = SVM_predictEEPROM(sensor, sizeof(sensor)/sizeof(float));
  Serial.println(ret);
}

void loop() {
}
