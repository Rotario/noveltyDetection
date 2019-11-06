#include "noveltyDetection.h"

void setup(){
	Serial.begin(9600);
	while(!Serial);

	Serial.print("Reading model from SD Card into EEPROM...");
	//Default filenames are "svm.mod" and "svm.par" for the model and parameter files, respectively
	if (SVM_readModelFromSD(SVM_MODEL_FILENAME, SVM_SCALE_PARAMETERS_FILENAME) != 0){
		Serial.println("failed");
	} else {
		Serial.println("success!");
	}
}

void loop(){
//Assumes 3 features
float data[] = {analogRead(A0), analogRead(A1), analogRead(A2)};
float ret = SVM_predictEEPROM(data, 3);
int recognisedClass;
if (ret > 0) {
    Serial.println("Not Novelty");
    recognisedClass = 1;
  } else {
    Serial.println("Novelty");
    recognisedClass = -1;
  }
delay(1000);
}

