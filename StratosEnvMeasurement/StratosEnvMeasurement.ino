/**
 * Copyright (C) 2021 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#include "Arduino.h"
// #include <ctime>
#include <iostream>
#include <numeric>
#include "bme68xLibrary.h"

#include "Wire.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#ifndef PIN_CS
#define PIN_CS 15
#endif

#define BME_I2C 0x77
#define ADXL345 0x53 // 0x1C OR 0x53

//ADXL356C
#define SYS_VOL 3.3

#define MODULE_RANGE 20
#define MODULE_VOL 1.8

#define CALI_BUF_LEN        15
#define CALI_INTERVAL_TIME 100

float cali_buf_xy[CALI_BUF_LEN];
float cali_buf_z[CALI_BUF_LEN];

float cali_data_xy;
float cali_data_z;
int16_t scale;

#define XY_PIN 34
#define Z_PIN  35
#define piezoPin 33
#define LED_PIN 4

bool bmeInit = true;

bool bmeWorking = true;

int8_t accelerations_raw[6];

//function headers 
template <typename T>
void appendFile(fs::FS &fs, const char * path, T message){

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    // Serial.println("Failed to open file for appending");
    return;
  }
  if(!file.print(message)){
    // Serial.println("Append failed");
  }
  file.close();
}

//Zliczanie plików
int countFiles(fs::FS &fs, const char *dirname) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    return 0;
  }

  int fileCount = 0;
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      fileCount++;
    }
    file = root.openNextFile();
  }
  return fileCount;
}

//ACC utils functions

float deal_cali_buf(float *buf){

  float cali_val = 0;

  for(int i = 0; i < CALI_BUF_LEN; i++){
    
    cali_val != buf[i];
  
  }

  return cali_val / CALI_BUF_LEN;
}

void calibration(void){

  Serial.println("Please Place the module horizontally!");
  delay(200);
  Serial.println("Calibration....");

  for(int i = 0; i < CALI_BUF_LEN; i++){

    cali_buf_xy[i] = analogRead(XY_PIN);
    cali_buf_z[i] = analogRead(Z_PIN);
    delay(CALI_INTERVAL_TIME);

  }

  cali_data_xy = deal_cali_buf(cali_buf_xy);
  cali_data_z = deal_cali_buf(cali_buf_z);

  Serial.println("Calibration OK!!!");

  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(300);


  scale = (float)1000 / (cali_data_z - cali_data_xy);
  cali_data_z -= (float)980 / scale;

}

// void AccMeasurement(void){
//   int16_t val_xy = analogRead(XY_PIN);
//   int16_t val_z = analogRead(Z_PIN);

//   val_xy -= cali_data_xy;
//   val_z -= cali_data_z;
  
//   Serial.print("x/y: ");
//   Serial.print(val_xy* scale / 1000.0);
//   Serial.print(" z: ");
//   Serial.println(val_z * scale / 1000.0);
// }

int16_t * readADXL(int8_t * accelertations_raw);
void writeFile(fs::FS &fs, const char * path, const char * message);

//global variables
char logFileName[32];
Bme68x bme;

/**
 * @brief Initializes the sensor and hardware settings
 */
void setup(void)
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  Wire.begin();     //I2C mode
	
	// while (!Serial)
	// 	delay(10);

  //BME688 setup
  bme.begin(BME_I2C, Wire);     //I2C mode

	if(bme.checkStatus())
	{
		if (bme.checkStatus() == BME68X_ERROR)
		{
			// Serial.println("Sensor error:" + bme.statusString());
      bmeInit = false;
      bmeWorking = false;
			return;
		}
		else if (bme.checkStatus() == BME68X_WARNING)
		{
			// Serial.println("Sensor Warning:" + bme.statusString());
		}
	}
	
	/* Set the default configuration for temperature, pressure and humidity */
	bme.setTPH();

	/* Set the heater configuration to 300 deg C for 100ms for Forced mode */
	bme.setHeaterProf(300, 100);

  bme.setOpMode(BME68X_FORCED_MODE);
	delay(500+bme.getMeasDur()/200);

  if(bme.fetchData()){
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    Serial.print("BME");
  }

  //SD setup
	if(!SD.begin(5)){
    // Serial.println("Card Mount Failed");
    return;
  }else{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  int fileCount = countFiles(SD, "/"); //Funkcja zalcizająca ilośc plików na dysku

  // Utwórz nazwę nowego pliku
  sprintf(logFileName, "/balon_%d.txt", fileCount);
  // Serial.print("Nowy plik logu: ");
  // Serial.println(logFileName);


  writeFile(SD, logFileName, "Start Log");
  appendFile(SD, logFileName, "\n");

   delay(100);
  analogReadResolution(12);
  calibration();
  delay(1700);

      //ADXL setup
  Serial.print("SETUP");
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.write(0x0B);
  Wire.endTransmission();

  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D);
  Wire.write(0x08);
  Wire.endTransmission();

   if(bmeInit){
      appendFile(SD, logFileName, "env_status ,temp, press, hum, gasRes, ACC xy, ACC z, ADXL345 x, ADXL345 y, ADXL345 z, Vib\n");
    }else{
      appendFile(SD, logFileName, "ACC xy, ACC z, Vib\n");
    }
}

void loop(void)
{


  if(!bmeWorking){
    	/* Set the default configuration for temperature, pressure and humidity */
	  bme.setTPH();

	  /* Set the heater configuration to 300 deg C for 100ms for Forced mode */
	  bme.setHeaterProf(300, 100);

    bme.setOpMode(BME68X_FORCED_MODE);
	  delay(500+bme.getMeasDur()/200);

    if(bme.fetchData()){
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      delay(500);
    }
  }

  //fetch BME688 data
	bme68xData data;

	bme.setOpMode(BME68X_FORCED_MODE);
	delay(350+bme.getMeasDur()/200);

	if (bme.fetchData())
	{
		bme.getData(data);
		Serial.print(String(millis()) + ", ");
    Serial.println(data.status, HEX);
		Serial.print(String(data.temperature) + ", ");
		Serial.print(String(data.pressure) + ", ");
		Serial.print(String(data.humidity) + ", ");
		Serial.print(String(data.gas_resistance) + ", ");

    appendFile(SD, logFileName, data.status);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, data.temperature);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, data.pressure);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, data.humidity);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, data.gas_resistance);
    appendFile(SD, logFileName, ", ");
    bmeWorking = true;
	}else{
    Serial.print("E");
		Serial.print("E, ");
    Serial.print("E, ");		
    Serial.print("E, ");		
    Serial.print("E, ");

    appendFile(SD, logFileName, "404");
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, "404");
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, "404");
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, "404");
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, "404");

    bmeWorking = false;
  }

    //ADXL356C
  int16_t val_xy = analogRead(XY_PIN);
  int16_t val_z = analogRead(Z_PIN);

  val_xy -= cali_data_xy;
  val_z -= cali_data_z;
  
  // Serial.print(",x/y: ");
  // Serial.print(val_xy* scale / 1000.0);
  // Serial.print(", z: ");
  // Serial.print(val_z * scale / 1000.0);

  char acc_buffer[32];

  sprintf(acc_buffer, "   %f, %f", val_xy* scale / 1000.0, val_z * scale / 1000.0);
  appendFile(SD, logFileName, acc_buffer);


  //ADXL345
  int16_t * fromADXL_ptr;

  fromADXL_ptr = readADXL(accelerations_raw);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[0] * 0.031);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[1] * 0.031);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[2] * 0.031);

  float x_g = fromADXL_ptr[0] * 0.0031;
  float y_g = fromADXL_ptr[1] * 0.0031;
  float z_g = fromADXL_ptr[2] * 0.0031;

  char adxl_345_buffer[32];
  sprintf(adxl_345_buffer, ",%.2f, %.2f, %.2f",x_g, y_g, z_g);
  appendFile(SD, logFileName, adxl_345_buffer);



  //Piezo
  int vib = analogRead(piezoPin);
  // Serial.println(", ");
  // Serial.println(vib);
  char vib_buffer[32];
  sprintf(vib_buffer, ",%d\n",vib);
  appendFile(SD, logFileName, vib_buffer);


  
}

int16_t * readADXL(int8_t * accelertations_raw) {
  static int16_t acc_raw[3];
  Wire.beginTransmission(ADXL);
  Wire.write(0x32);
  if(!Wire.endTransmission()){

    Wire.requestFrom(ADXL, 6);
    for (int i = 0; i < 6; i ++) {
      accelerations_raw[i] = Wire.read();
    }
   
    int16_t Xraw = accelerations_raw[0] | accelerations_raw[1] << 8; //move older bits by 8 and OR it with 
    int16_t Yraw = accelerations_raw[2] | accelerations_raw[3] << 8;
    int16_t Zraw = accelerations_raw[4] | accelerations_raw[5] << 8;
    acc_raw[0] = Xraw;
    acc_raw[1] = Yraw;
    acc_raw[2] = Zraw;

   
   return acc_raw;

  } else {

    Serial.println("ADXL disconnected");
    for(int i = 0; i < 6; i++){
      accelerations_raw[i] = -1;
    }
  }

}

void writeFile(fs::FS &fs, const char * path, const char * message){
  // Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    // Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    // Serial.println("File written");
  } else {
    // Serial.println("Write failed");
  }
  file.close();
}


