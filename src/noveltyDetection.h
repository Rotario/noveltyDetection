/*
  noveltyDetection.h - Library for running SVM 
  classification algorithms given an SVM model on SD
  card or SPIFFS.
  Created by Rowan Easter-Robinson, August 23, 2018.
  Released into the public domain.
*/

#ifndef NOVELTY_DETECTION_H
#define NOVELTY_DETECTION_H

#include <Arduino.h>

#ifdef ARDUINO_ARCH_ESP8266
  //FS uses SPIFFS, all filenames must start with /
  #define SVM_MODEL_FILENAME "/svm.mod"
  #define SVM_SCALE_PARAMETERS_FILENAME "/svm.par"
  #include <FS.h>
#else
  //Used to read the right file from the SD Card
  #define SVM_MODEL_FILENAME "svm.mod"
  #define SVM_SCALE_PARAMETERS_FILENAME "svm.par"
  #include <SD.h>
#endif

#include <EEPROM.h>


//Max number of features
#define SVM_MAX_VEC_DIM 10
//Used for checking the files on SD have the correct params
#define SVM_TYPE "one_class"
#define KERNEL_TYPE "rbf"
#define NR_CLASS 2
#define SD_BUF_LENGTH 512 //newline buffer to read SVM params from SD cardshouldn't exceed 500

struct SVM_parameterStruct {
  uint32_t nVecDims;
  uint32_t totalSVs;
  float gamma;
  float rho;
};

enum eepromAddresses {
  aSVM_START, //Add stuff before this if you want to shift the SVM data forwards in the EEPROM
  aSVM_SCALE_PARAMS_START = aSVM_START + sizeof(SVM_parameterStruct),
  aSVM_SV_START = aSVM_SCALE_PARAMS_START + sizeof(float) * 2 * SVM_MAX_VEC_DIM, //sizeof float * upper+lower * 10 max vector dimensions
  aSVM_SV_END = 2000,
};

const int maxSVs = (aSVM_SV_END - aSVM_SV_START) / sizeof(float);

//Std scale params, these are recommended by LIBSVM
const int scalePar[] = { -1, 1};

float SVM_predictEEPROM(float* sensor, uint8_t nSensor);

void SVM_scaleEEPROM(const float* sensor, float* scaledSensor, uint8_t vecDim, eepromAddresses eepromAddr);

int SVM_readModelFromSD(const char * modelFile, const char * scaleParamsFile);

int SVM_readModelFromSPIFFS(const char * modelFile, const char * scaleParamsFile);

inline float SVM_rbfKernel(uint16_t startAddr, float* v, uint8_t vecDims, float gamma);

void readToCharCode(File * f, char c, int bufSize, char * buf);


#endif
