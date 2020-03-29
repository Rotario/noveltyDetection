/*
  noveltyDetection.cpp - Library for running SVM 
  classification algorithms given an SVM model on SD
  card or SPIFFS.
  Created by Rowan Easter-Robinson, August 23, 2018.
  Released into the public domain.
*/

#include <Arduino.h>
#include "noveltyDetection.h"

float SVM_predictEEPROM(float* sensor, uint8_t nSensor) {
  uint16_t i;
  
  SVM_parameterStruct svm;
  EEPROM.get(aSVM_START, svm); //get parameters from EEPROM

  //sanity checks, no of given vectors over max? size of given vector dims same as those expected in SVM model?
  if (svm.nVecDims > SVM_MAX_VEC_DIM) {
    Serial.println("NVec dims bigger than max");
    return -1;
  }
  if (nSensor != svm.nVecDims) {
    Serial.println("NVec Dims different to those in SVM model");
    return -1;
  }

  float scaledSensor[SVM_MAX_VEC_DIM];
  SVM_scaleEEPROM(sensor, scaledSensor, svm.nVecDims, aSVM_SCALE_PARAMS_START);
  for (i = 0; i < svm.nVecDims; i++) {
    char buf[50];
    snprintf(buf, 50, "Scaled Values[%i]: %.5f\n", i, scaledSensor[i]);
    Serial.print(buf);
  }

  //One class implementation
  float sum = 0;

  //Iterate through ALL Support vectors
  for (i = 0; i < svm.totalSVs; i++) {
    float sv_coef;
    int startAddr = aSVM_SV_START + (i * (svm.totalSVs * sizeof(float))); //init start address for this loop
    //Serial.printf("Weighting Addr = %i\t", startAddr);
    EEPROM.get(startAddr, sv_coef); //get weighting from address
    //Serial.printf("Weighting = %.5f\t", sv_coef);

    sum += sv_coef * SVM_rbfKernel(startAddr, scaledSensor, svm.nVecDims, svm.gamma); //SV = pointer to first in support vector dimension "weighting" = 1:...
    //Serial.println();
  }

  /*if (sum > 0) {
    recognizedClass = 1;
  } else {
    recognizedClass = -1;
  }*/
  return sum;
}

void SVM_scaleEEPROM(const float* sensor, float* scaledSensor, uint8_t vecDim, eepromAddresses eepromAddr) {
  int startAddr;
  float low;
  float high;
  for (int p = 0; p < vecDim; p++) {
    startAddr = aSVM_SCALE_PARAMS_START + (p * (2 * sizeof(float))); //init start address for this loop, 2 values for every vector dimension
    //Serial.printf("Start address for this loop = %i\t", startAddr);
    //Serial.printf("Addr = %i \t", startAddr);
    EEPROM.get(startAddr, low);
    //Serial.printf("Lower value = %f\t", low); //Cast to float as atof returns double
    //Serial.printf("Addr = %i \t", startAddr + 1 * sizeof(float));
    EEPROM.get(startAddr + sizeof(float), high);
    //Serial.printf("Upper Value value = %f\t", high); //Cast to float as atof returns double
    scaledSensor[p] = (float)scalePar[0] + ((float)scalePar[1] - ((float)scalePar[0])) * ((float)sensor[p] - (float)low) / ((float)high - (float)low);
  }
  //Serial.println();
}

inline float SVM_rbfKernel(uint16_t startAddr, float* v, uint8_t vecDims, float gamma) {
  //Serial.printf("RBF Kernel: ");
  float result = 0;
  float u;
  for (int i = 0; i < vecDims; i++) {
    //Serial.printf("%i\t",startAddr + ((i + 1) * sizeof(float)));
    EEPROM.get(startAddr + ((i + 1) * sizeof(float)), u);
    //Serial.printf("%i:%.5f\t", i + 1, u);
    //Serial.printf("%f - %f\t", u[i], v[i]);
    float temp = u - v[i];
    result += temp * temp;
  }
  //Serial.printf("Result = %.10f\n", exp(-gamma * result));
  return exp(-gamma * result);
}

//TODO: Support ESP8266 (and maybe others) with Precompiler-controlled EEPROM.ommit()
//TODO: make another function to read from JSON and send as JSON from memory
//TODO: nok if model not read
//Todo: more checks eg n svs = no of updates addresses, n vec dikms are asll same #9first + last)
//returns 0 for success, anything else is fail
#ifdef __SD_H__
int SVM_readModelFromSD(const char * modelFile, const char * scaleParamsFile) {
  SVM_parameterStruct svm;
  File f;
  uint16_t i = 0;
  char sdBuf[SD_BUF_LENGTH];//buff to fill newline with, shouldn't exceed 500
  char * pch;
  Serial.print(F("Reading SVM model file..."));
  Serial.print(modelFile);
  if (SD.exists(modelFile) && SD.exists(scaleParamsFile)) {
    f = SD.open(modelFile, FILE_READ);
    if (f) {
      Serial.println(F("success"));
      while (f.available()) {
        readToCharCode(&f, 0x0A, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until newline LF
        pch = strtok(sdBuf, " "); //Split into spaces
        if (pch != NULL) {
          if (strcmp("svm_type", pch) == 0 ) {
            Serial.println(F("Found svm_type"));
            Serial.println(pch);
            pch = strtok(NULL, " "); //Read string after space;
            Serial.println(pch);
            if (strcmp(SVM_TYPE, pch) != 0) {
              Serial.println(F("Incorrect svm_type, must be one_class"));
              f.close();
              return -1;
            }

          } else if (strcmp("kernel_type", pch) == 0 ) {
            Serial.println(F("Found kernel_type"));
            pch = strtok(NULL, " "); //Read string after space;
            if (strcmp(KERNEL_TYPE, pch) != 0) {
              Serial.println(F("Incorrect kernel_type, must be rbf"));
              f.close();
              return -1;
            }

          } else if (strcmp("gamma", pch) == 0 ) {              //Put parameters in EEPROM
            Serial.println(F("Found gamma"));
            pch = strtok(NULL, " "); //Read string after space;
            svm.gamma = atof(pch);

          } else if (strcmp("nr_class", pch) == 0 ) {
            Serial.println(F("Found nr_class"));
            pch = strtok(NULL, " "); //Read string after space;
            if (atoi(pch) != 2) {
              Serial.println(F("Incorrect nr_class, must be 2"));
              f.close();
              return -1;
            }

          } else if (strcmp("total_sv", pch) == 0 ) {
            Serial.println(F("Found total_sv"));
            pch = strtok(NULL, " "); //Read string after space;
            if (atoi(pch) > maxSVs) {
              Serial.print(F("Total SVs larger than max of "));
	      Serial.println(maxSVs);
              f.close();
              return -1;
            }
            svm.totalSVs = atoi(pch);

          } else if (strcmp("rho", pch) == 0 ) {
            Serial.println(F("Found rho"));
            pch = strtok(NULL, " "); //Read string after space;
            svm.rho = atof(pch);

          } else if (strcmp("SV", pch) == 0 ) { //Always last data in file
            //EEPROM writes inside the reading loop because if there's a problem it'll return before this always
            Serial.println(F("Found SV"));
            //iterate through number of SVs, writing to eeprom each time
            for (i = 0; i < svm.totalSVs; i++) {
              readToCharCode(&f, 0x0A, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until newline CR found, dump out LF
              if (i == maxSVs) {
                Serial.print(F("Too many support vectors"));
                f.close();
                return -1; //Check address not greater than max reserved
              }

              pch = strtok(sdBuf, " :");
              Serial.print(F("SV Weighting: "));
	      Serial.println(atof(pch));
              //Calc EEPROM address and cast parsed value to float from double
              int startAddr = aSVM_SV_START + (i * (svm.totalSVs * sizeof(float))); //init start address for this loop
              Serial.print(F("Start address for this loop) = "));
	      Serial.println(startAddr);
              EEPROM.put(startAddr, (float) atof(pch)); //write weighting to address, //Cast to float as atof returns double
              pch = strtok(NULL, " :");
              while (pch != NULL) {
                //Iterate through number of dimensions
                Serial.print(F("Dim number: "));
		Serial.println(atoi(pch));
                svm.nVecDims = atoi(pch);
                if (svm.nVecDims >= SVM_MAX_VEC_DIM) {
                  f.close();
                  return -1;
                }
                pch = strtok(NULL, " :"); //read out dimension SV number (To next : )
                Serial.print(F("Dim value = "));
		Serial.println((float) atof(pch)); //Cast to float as atof returns double
                Serial.print(F("Addr = "));
		Serial.println(startAddr + (svm.nVecDims*sizeof(float)));
                EEPROM.put(startAddr + (svm.nVecDims*sizeof(float)), (float) atof(pch));
                pch = strtok(NULL, " :"); //read out dimension SV value (To next SPACE (0x20))
              }
            }
          } else {
            Serial.println(F("Not Recognised:"));
            Serial.print(sdBuf);
            Serial.println();
            return -1;
          }
        }

      }
      f.close();
      //Didn't error out during file read so write all parameters to EEPROM then check visually
      EEPROM.put(aSVM_START, svm);
      Serial.print(F("N Vector Dims =")); Serial.println(svm.nVecDims);
      Serial.print(F("Total SVs =")); Serial.println(svm.totalSVs);
      Serial.print(F("Gamma =")); Serial.println(svm.gamma);
      Serial.print(F("Rho =")); Serial.println(svm.rho);
      Serial.print(F("SVM File Parsing finished!"));
      //SD.remove(SVM_MODEL_FILENAME);
    } else {
      Serial.println(F("...failed"));
      return -1; //dont read SVM parameters if svm model file failed
    }

    f = SD.open(scaleParamsFile, FILE_READ);
    if (f) {
      Serial.println(F("success"));
      while (f.available()) {
        readToCharCode(&f, 0x0A, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF
        pch = strtok(sdBuf, " "); //Split into spaces
        if (pch != NULL) {
          if (strcmp("x", pch) == 0 ) { //Always last data in file
            //EEPROM writes inside the reading loop because if there's a problem it'll return before this always
            Serial.println(F("Found x"));
            //iterate through number of SVs, writing to eeprom each time
            readToCharCode(&f, 0x0A, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until newline CR found, dump out LF
            pch = strtok(sdBuf, " ");
            Serial.print(F("Scale Lower Value: "));
	    Serial.println(atof(pch));
            pch = strtok(NULL, " ");
            Serial.print(F("Scale Upper Value: "));
	    Serial.print(atof(pch));

            for (i = 0; i < svm.nVecDims; i++) { //should always be less than 10. if not then never gets to this point, returns in svm.mod file parsing stage
              readToCharCode(&f, 0x0A, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF

              //Calc EEPROM address and cast parsed value to float from double
              int startAddr = aSVM_SCALE_PARAMS_START + (i * (2 * sizeof(float))); //init start address for this loop, 2 values for every vector dimension
              //Serial.printf("Start address for this loop = %i\t", startAddr);
              pch = strtok(sdBuf, " ");
              //Iterate through number of dimensions
              //Serial.printf("Dim number: %i\t", atoi(pch));

              pch = strtok(NULL, " "); //read out dimension SV number (To next : )
              //Serial.printf("Addr = %i \t", startAddr);
              //Serial.printf("Lower value = %f\t", (float) atof(pch)); //Cast to float as atof returns double
              EEPROM.put(startAddr, (float) atof(pch));

              pch = strtok(NULL, " "); //read out dimension SV value (To next SPACE (0x20))
              //Serial.printf("Addr = %i \t", startAddr + 1 * sizeof(float));
              //Serial.printf("Upper Value value = %f\t", (float) atof(pch)); //Cast to float as atof returns double
              EEPROM.put(startAddr + sizeof(float), (float) atof(pch));

              Serial.println();
            }
          } else {
            Serial.println(F("Not Recognised:"));
            Serial.print(sdBuf);
            Serial.println();
            return -1;
          }
        }

      }
      f.close();
      //Didn't error out during file read so write all parameters to EEPROM then check visually
      Serial.print(F("Scale Parameters File Parsing finished!"));
      //SD.remove(scaleParamsFile);
    } else {
      Serial.println(F("...failed"));
      return -1;
    }

  } else {
    Serial.println(F("...doesn't exist"));
    return -2;
  }
  Serial.println(F("...done"));
  return 0;
}
#endif

#ifdef FS_H
int SVM_readModelFromSPIFFS(const char * modelFile, const char * scaleParamsFile) {
  SVM_parameterStruct svm;
  File f;
  uint16_t i = 0;
  char sdBuf[SD_BUF_LENGTH];//buff to fill newline with, shouldn't exceed 500
  char * pch;
  Serial.print(F("Reading SVM model file..."));
  Serial.print(modelFile);
  if (SPIFFS.exists(modelFile) && SPIFFS.exists(scaleParamsFile)) {
    f = SPIFFS.open(modelFile, "r");
    if (f) {
      Serial.println(F("success"));
      while (f.available()) {
        readToCharCode(&f, 0x0D, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF
        if (f.peek() == 0x0A){
          Serial.println("dumping out LF if exists");
          f.read();
        }
        pch = strtok(sdBuf, " "); //Split into spaces
        if (pch != NULL) {
          if (strcmp("svm_type", pch) == 0 ) {
            Serial.println(F("Found svm_type"));
            Serial.println(pch);
            pch = strtok(NULL, " "); //Read string after space;
            Serial.println(pch);
            if (strcmp(SVM_TYPE, pch) != 0) {
              Serial.print(F("Incorrect svm_type, must be "));Serial.println(SVM_TYPE);
              Serial.println(pch);
              f.close();
              return -1;
            }

          } else if (strcmp("kernel_type", pch) == 0 ) {
            Serial.println(F("Found kernel_type"));
            pch = strtok(NULL, " "); //Read string after space;
            if (strcmp(KERNEL_TYPE, pch) != 0) {
              Serial.println(F("Incorrect kernel_type, must be rbf"));
              f.close();
              return -1;
            }

          } else if (strcmp("gamma", pch) == 0 ) {              //Put parameters in EEPROM
            Serial.println(F("Found gamma"));
            pch = strtok(NULL, " "); //Read string after space;
            svm.gamma = atof(pch);

          } else if (strcmp("nr_class", pch) == 0 ) {
            Serial.println(F("Found nr_class"));
            pch = strtok(NULL, " "); //Read string after space;
            if (atoi(pch) != 2) {
              Serial.println(F("Incorrect nr_class, must be 2"));
              f.close();
              return -1;
            }

          } else if (strcmp("total_sv", pch) == 0 ) {
            Serial.println(F("Found total_sv"));
            pch = strtok(NULL, " "); //Read string after space;
            if (atoi(pch) > maxSVs) {
              Serial.print(F("Total SVs larger than max of "));
	            Serial.println(maxSVs);
              f.close();
              return -1;
            }
            svm.totalSVs = atoi(pch);

          } else if (strcmp("rho", pch) == 0 ) {
            Serial.println(F("Found rho"));
            pch = strtok(NULL, " "); //Read string after space;
            svm.rho = atof(pch);

          } else if (strcmp("SV", pch) == 0 ) { //Always last data in file
            //EEPROM writes inside the reading loop because if there's a problem it'll return before this always
            Serial.println(F("Found SV"));
            //iterate through number of SVs, writing to eeprom each time
            for (i = 0; i < svm.totalSVs; i++) {
              readToCharCode(&f, 0x0D, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until newline CR found, dump out LF
              if (f.peek() == 0x0A){
                Serial.println("dumping out LF if exists");
                f.read();
              }
                //dump out LF
              if (i == maxSVs) {
                Serial.print(F("Too many support vectors"));
                f.close();
                return -1; //Check address not greater than max reserved
              }

              pch = strtok(sdBuf, " ");
              Serial.print(F("SV Weighting: "));
	            Serial.println(atof(pch));
              //Calc EEPROM address and cast parsed value to float from double
              int startAddr = aSVM_SV_START + (i * (svm.totalSVs * sizeof(float))); //init start address for this loop
              Serial.print(F("Start address for this loop) = "));
	            Serial.println(startAddr);
              EEPROM.put(startAddr, (float) atof(pch)); //write weighting to address, //Cast to float as atof returns double
              pch = strtok(NULL, " :");
              while (pch) {
                //Iterate through number of dimensions
                Serial.print(F("Dim number: "));
		            Serial.println(atoi(pch));
                svm.nVecDims = atoi(pch);
                if (svm.nVecDims >= SVM_MAX_VEC_DIM) {
                  Serial.println(F("No of dimensions greater than max defined in SVM_MAX_VEC_DIM"));
                  f.close();
                  return -1;
                }
                pch = strtok(NULL, " :"); //read out dimension SV number (To next : )
                Serial.print(F("Dim value = "));
		            Serial.println((float) atof(pch)); //Cast to float as atof returns double
                Serial.print(F("Addr = "));
		            Serial.println(startAddr + (svm.nVecDims*sizeof(float)));
                EEPROM.put(startAddr + (svm.nVecDims*sizeof(float)), (float) atof(pch));
                pch = strtok(NULL, " :"); //read out dimension SV value (To next SPACE (0x20))
              }
            }
          } else {
            Serial.println(F("Not Recognised:"));
            Serial.print(sdBuf);
            Serial.println();
            return -1;
          }
        }

      }
      f.close();
      //Didn't error out during file read so write all parameters to EEPROM then check visually
      EEPROM.put(aSVM_START, svm);
      Serial.print(F("N Vector Dims =")); Serial.println(svm.nVecDims);
      Serial.print(F("Total SVs =")); Serial.println(svm.totalSVs);
      Serial.print(F("Gamma =")); Serial.println(svm.gamma);
      Serial.print(F("Rho =")); Serial.println(svm.rho);
      Serial.print(F("SVM File Parsing finished!"));
      //SD.remove(SVM_MODEL_FILENAME);
    } else {
      Serial.println(F("...failed"));
      return -1; //dont read SVM parameters if svm model file failed
    }

    f = SPIFFS.open(scaleParamsFile, "r");
    if (f) {
      Serial.println(F("success"));
      while (f.available()) {
        readToCharCode(&f, 0x0D, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF
        if (f.peek() == 0x0A){
          Serial.println("dumping out LF if exists");
          f.read();
        }
        pch = strtok(sdBuf, " "); //Split into spaces
        if (pch != NULL) {
          if (strcmp("x", pch) == 0 ) { //Always last data in file
            //EEPROM writes inside the reading loop because if there's a problem it'll return before this always
            Serial.println(F("Found x"));
            //iterate through number of SVs, writing to eeprom each time
            readToCharCode(&f, 0x0D, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF
            if (f.peek() == 0x0A){
              Serial.println("dumping out LF if exists");
              f.read();
            } 
            pch = strtok(sdBuf, " ");
            Serial.print(F("Scale Lower Value: "));
	          Serial.println(atof(pch));
            pch = strtok(NULL, " ");
            Serial.print(F("Scale Upper Value: "));
	          Serial.print(atof(pch));

            for (i = 0; i < svm.nVecDims; i++) { //should always be less than 10. if not then never gets to this point, returns in svm.mod file parsing stage
              readToCharCode(&f, 0x0D, SD_BUF_LENGTH, sdBuf); //Fill messagebuffer with file until LF
              if (f.peek() == 0x0A){
                Serial.println("dumping out LF if exists");
                f.read();
              }
              //Calc EEPROM address and cast parsed value to float from double
              int startAddr = aSVM_SCALE_PARAMS_START + (i * (2 * sizeof(float))); //init start address for this loop, 2 values for every vector dimension
              Serial.printf("Start address for this loop = %i\t", startAddr);
              pch = strtok(sdBuf, " ");
              //Iterate through number of dimensions
              Serial.printf("Dim number: %i\t", atoi(pch));

              pch = strtok(NULL, " "); //read out dimension SV number (To next : )
              Serial.printf("Addr = %i \t", startAddr);
              Serial.printf("Lower value = %f\t", (float) atof(pch)); //Cast to float as atof returns double
              EEPROM.put(startAddr, (float) atof(pch));

              pch = strtok(NULL, " "); //read out dimension SV value (To next SPACE (0x20))
              Serial.printf("Addr = %i \t", startAddr + 1 * sizeof(float));
              Serial.printf("Upper Value value = %f\t", (float) atof(pch)); //Cast to float as atof returns double
              EEPROM.put(startAddr + sizeof(float), (float) atof(pch));

              Serial.println();
            }
          } else {
            Serial.println(F("Not Recognised:"));
            Serial.print(sdBuf);
            Serial.println();
            return -1;
          }
        }

      }
      f.close();
      EEPROM.commit();
      //Didn't error out during file read so write all parameters to EEPROM then check visually
      Serial.print(F("Scale Parameters File Parsing finished!"));
      //SD.remove(scaleParamsFile);
    } else {
      Serial.println(F("...failed"));
      return -1;
    }

  } else {
    Serial.println(F("...doesn't exist"));
    return -2;
  }
  Serial.println(F("...done"));
  return 0;
}
#endif

void readToCharCode(File * f, char c, int bufSize, char * buf) {
  int i = 0;
  if (f) {
    do {
      if (f->available()) {
        if (i < bufSize) {
          buf[i] = f->read();
          i++;
        } else {
          Serial.println(F("Specified char code not found, buffer filled"));
          return;
        }
      } else {
        Serial.println(F("Specified char code not found, EOF reached"));
        return;
      }
    } while (buf[i - 1] != c); //look for given char
    buf[i - 1] = '\0'; //Replace given char by null pointer
  } else {
    Serial.println(F("File needs opening before readToCharCode called"));
  }
}

