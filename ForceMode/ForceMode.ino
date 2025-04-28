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

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "DFRobot_LTR390UV.h"
DFRobot_LTR390UV ltr390(/*addr = */LTR390UV_DEVICE_ADDR, /*pWire = */&Wire);

#ifndef PIN_CS
#define PIN_CS 15
#endif

#define ADD_I2C 0x77
// #define ADXL 0x53 //accelerometer, 83 in DEC, alt. 0x1C, 0x53
// #define MCP 0x18 //termometr wewnątrz modułu

//ADXL356C
#define SERIAL Serial
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

// int successfulInits = 0; // Licznik poprawnych inicjalizacji
bool uvInit = true;
bool bmeInit = true;

bool bmeWorking = true;

//function headers 
template <typename T>
void appendFile(fs::FS &fs, const char * path, T message){

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(!file.print(message)){
    Serial.println("Append failed");
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

//UV utils functions
// Oblicz indeks UV w zakresie 0-11 (przybliżenie)
int mapUVIndex(uint32_t uv_raw) {
  // Skalowanie - dostosuj w zależności od typowego zakresu
  float scaled = (float)uv_raw / 1800.0 * 11.0;
  if (scaled > 11.0) scaled = 11.0;
  return (int)(scaled + 0.5);
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

  SERIAL.println("Please Place the module horizontally!");
  delay(200);
  SERIAL.println("Calibration....");

  for(int i = 0; i < CALI_BUF_LEN; i++){

    cali_buf_xy[i] = analogRead(XY_PIN);
    cali_buf_z[i] = analogRead(Z_PIN);
    delay(CALI_INTERVAL_TIME);

  }

  cali_data_xy = deal_cali_buf(cali_buf_xy);
  cali_data_z = deal_cali_buf(cali_buf_z);

  SERIAL.println("Calibration OK!!!");

  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);


  scale = (float)1000 / (cali_data_z - cali_data_xy);
  cali_data_z -= (float)980 / scale;

}

// void AccMeasurement(void){
//   int16_t val_xy = analogRead(XY_PIN);
//   int16_t val_z = analogRead(Z_PIN);

//   val_xy -= cali_data_xy;
//   val_z -= cali_data_z;
  
//   SERIAL.print("x/y: ");
//   SERIAL.print(val_xy* scale / 1000.0);
//   SERIAL.print(" z: ");
//   SERIAL.println(val_z * scale / 1000.0);
// }

// int16_t * readADXL(int8_t * accelertations_raw);
void writeFile(fs::FS &fs, const char * path, const char * message);
void testFileIO(fs::FS &fs, const char * path);
void createDir(fs::FS &fs, const char * path);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);

//global variables
char logFileName[32];
float intTemp;
Bme68x bme;


byte uv_data[2]; //pomocnicza do wczytania danych
float uv; //UV z czujnika cyfrowego
float analogUv; //UV z czujnika analogowego
float UVindex; //przeliczony indeks UV


// int8_t accelerations_raw[6]; //przyspieszenia x, y, z

/**
 * @brief Initializes the sensor and hardware settings
 */
void setup(void)
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  Wire.begin();     //I2C mode
	Serial.begin(115200);
	
	while (!Serial)
		delay(10);

  //BME688 setup
  bme.begin(ADD_I2C, Wire);     //I2C mode

	if(bme.checkStatus())
	{
		if (bme.checkStatus() == BME68X_ERROR)
		{
			Serial.println("Sensor error:" + bme.statusString());
      bmeInit = false;
      bmeWorking = false;
			return;
		}
		else if (bme.checkStatus() == BME68X_WARNING)
		{
			Serial.println("Sensor Warning:" + bme.statusString());
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
  }

  //SD setup
	if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }else{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  int fileCount = countFiles(SD, "/");

  // Utwórz nazwę nowego pliku
  sprintf(logFileName, "/stratos_%d.txt", fileCount);
  Serial.print("Nowy plik logu: ");
  Serial.println(logFileName);


  writeFile(SD, logFileName, "Start Log");
  appendFile(SD, logFileName, "\n");

   delay(100);
  analogReadResolution(12);
  calibration();
  delay(1700);
  SERIAL.print("Scale = ");
  SERIAL.println(scale);

  // //ADXL setup
  // Serial.print("SETUP");
  // Wire.beginTransmission(ADXL);
  // Wire.write(0x32); //rejestr zakresu danych
  // Wire.write(0x0B); //+- 16g, powinno wystarczyć
  // Wire.endTransmission();

  // Wire.beginTransmission(ADXL);
  // Wire.write(0x2D); //Power ctl
  // Wire.write(0x08); //Tryb pomiaru ciągłego
  // Wire.endTransmission();


  //UV setup
  if(ltr390.begin() != 0){
    Serial.println(" Sensor initialize failed!!");
    uvInit = false;
    delay(100);
  }else{
    Serial.println(" Sensor  initialize success!!");
    ltr390.setALSOrUVSMeasRate(ltr390.e18bit,ltr390.e100ms);//18-bit data, sampling time of 100ms 
    ltr390.setALSOrUVSGain(ltr390.eGain3);//Gain of 3
    ltr390.setMode(ltr390.eUVSMode);//Set UV mode 

    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(200);

  }

  if(uvInit){
    if(bmeInit){
      appendFile(SD, logFileName, "env_status ,temp, press, hum, gasRes, ACC xy, ACC z, Vib, UV\n");
    }else{
      appendFile(SD, logFileName, "ACC xy, ACC z, Vib, UV\n");
    }
  }else{
    if(bmeInit){
      appendFile(SD, logFileName, "env_status ,temp, press, hum, gasRes, ACC xy, ACC z, Vib\n");
    }else{
      appendFile(SD, logFileName, "ACC xy, ACC z, Vib\n");
    }
  }
}

void loop(void)
{

  // //fetch ADXL data
  // int16_t * fromADXL_ptr;

  // fromADXL_ptr = readADXL(accelerations_raw);

  // appendFile(SD, logfile, fromADXL_ptr[0]);
  // appendFile(SD, logfile, ", ");
  // appendFile(SD, logfile, fromADXL_ptr[1]);
  // appendFile(SD, logfile, ", ");
  // appendFile(SD, logfile, fromADXL_ptr[2]);
  // appendFile(SD, logfile, ", ");

  // Serial.print("Acceleration (x, y, z): ");
  //  Serial.print( "Xraw = ");
  //  Serial.print(fromADXL_ptr[0] * 0.031);
  //  Serial.print( "Yraw = ");
  //  Serial.print(fromADXL_ptr[1] * 0.031);
  //  Serial.print( "Zraw = ");
  //  Serial.print(fromADXL_ptr[2] * 0.031);
  //  Serial.print("\n");


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
	delay(200+bme.getMeasDur()/200);

	if (bme.fetchData())
	{
		bme.getData(data);
		// Serial.print(String(millis()) + ", ");
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
  // int16_t val_xy = analogRead(XY_PIN);
  // int16_t val_z = analogRead(Z_PIN);

  // val_xy -= cali_data_xy;
  // val_z -= cali_data_z;
  int16_t val_xy = analogRead(XY_PIN);
  int16_t val_z = analogRead(Z_PIN);

  val_xy -= cali_data_xy;
  val_z -= cali_data_z;
  
  SERIAL.print(",x/y: ");
  SERIAL.print(val_xy* scale / 1000.0);
  SERIAL.print(", z: ");
  SERIAL.print(val_z * scale / 1000.0);

  char acc_buffer[32];

  sprintf(acc_buffer, "   %f, %f", val_xy* scale / 1000.0, val_z * scale / 1000.0);
  appendFile(SD, logFileName, acc_buffer);



  //Piezo
  int vib = analogRead(piezoPin);
  SERIAL.println(", ");
  SERIAL.println(vib);
  char vib_buffer[32];
  sprintf(vib_buffer, ",%d",vib);
  appendFile(SD, logFileName, vib_buffer);


  //UV

  if(uvInit){
    uint32_t data_uv = 0;
    data_uv= ltr390.readOriginalData();//Get UV raw data
    int uv_index = mapUVIndex(data_uv);
    SERIAL.println(", ");
    Serial.println(data_uv);
    char uv_buffer[32];
    sprintf(uv_buffer, ", %d", uv_index);
    appendFile(SD, logFileName, uv_buffer);
    appendFile(SD, logFileName, "\n");
  }

  
}

// int16_t * readADXL(int8_t * accelertations_raw) {
//   static int16_t acc_raw[3];
//   Wire.beginTransmission(ADXL);
//   Wire.write(0x32);
//   if(!Wire.endTransmission()){

//     Wire.requestFrom(ADXL, 6);
//     for (int i = 0; i < 6; i ++) {
//       accelerations_raw[i] = Wire.read();
//     }
   
//     int16_t Xraw = accelerations_raw[0] | accelerations_raw[1] << 8; //move older bits by 8 and OR it with 
//     int16_t Yraw = accelerations_raw[2] | accelerations_raw[3] << 8;
//     int16_t Zraw = accelerations_raw[4] | accelerations_raw[5] << 8;
//     acc_raw[0] = Xraw;
//     acc_raw[1] = Yraw;
//     acc_raw[2] = Zraw;

   
//    return acc_raw;

//   } else {

//     Serial.println("ADXL disconnected");
//     for(int i = 0; i < 6; i++){
//       accelerations_raw[i] = -1;
//     }
//   }

// }


void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}


void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

