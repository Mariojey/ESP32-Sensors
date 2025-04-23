/**
 * Copyright (C) 2021 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */

#include "Arduino.h"
#include <ctime>
#include <iostream>
#include <numeric>
#include "bme68xLibrary.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// #include "DFRobot_LTR390UV.h"
// DFRobot_LTR390UV ltr390(/*addr = */LTR390UV_DEVICE_ADDR, /*pWire = */&Wire);

#ifndef PIN_CS
#define PIN_CS 15
#endif

#define ADD_I2C 0x77
// #define ADXL 0x53 //accelerometer, 83 in DEC, alt. 0x1C, 0x53
// #define MCP 0x18 //termometr wewnątrz modułu

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

// int16_t * readADXL(int8_t * accelertations_raw);
void writeFile(fs::FS &fs, const char * path, const char * message);
void testFileIO(fs::FS &fs, const char * path);
void createDir(fs::FS &fs, const char * path);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);

//global variables
const char* logfile = "/LOGFILE.txt";
float intTemp;
Bme68x bme;


// byte uv_data[2]; //pomocnicza do wczytania danych
// float uv; //UV z czujnika cyfrowego
// float analogUv; //UV z czujnika analogowego
// float UVindex; //przeliczony indeks UV


// int8_t accelerations_raw[6]; //przyspieszenia x, y, z

/**
 * @brief Initializes the sensor and hardware settings
 */
void setup(void)
{
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

  //SD setup
	if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  writeFile(SD, logfile, "Start Log");
  appendFile(SD, logfile, "\n");

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

// //MCP setup
//   Serial.print("MCP setup");
//   Wire.beginTransmission(MCP);
//   Wire.write(0x05); //CONFIG
//   Wire.write(0x00); //pomiar ciągły
//   Wire.endTransmission();


  // //UV setup
  // while(ltr390.begin() != 0){
  //   Serial.println(" Sensor initialize failed!!");
  //   delay(1000);
  // }
  // Serial.println(" Sensor  initialize success!!");
  // ltr390.setALSOrUVSMeasRate(ltr390.e18bit,ltr390.e100ms);//18-bit data, sampling time of 100ms 
  // ltr390.setALSOrUVSGain(ltr390.eGain3);//Gain of 3
  // ltr390.setMode(ltr390.eUVSMode);//Set UV mode 

  appendFile(SD, logfile, "accX, accY, accZ, intTemp, extTemp, extHum, extPress, gasRes, extUV\n");
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

  // //fetch MCP (internal temperature) data
  // if(intTemp==-1){

  // Wire.beginTransmission(MCP);
  // Wire.write(0x05); //CONFIG
  // Wire.write(0x00); //pomiar ciągły
  // Wire.endTransmission();
  // } 

  // Wire.beginTransmission(MCP);
  // Wire.write(0x05); //rejestr temperatury bezwzględnej
  // if(!Wire.endTransmission()){
  //   Wire.requestFrom(MCP, 2);

  //   uint16_t temp = Wire.read() << 8 | Wire.read();
  //   intTemp = (temp & 0x0FFF) / 16.0;
  //   if (temp & 0x1000) intTemp -= 256;

  //   Serial.print("Internal temperature: ");
  //   Serial.print(intTemp);
  //   Serial.println(" *C");
  //   appendFile(SD, logfile, intTemp);
  //   appendFile(SD, logfile, ", ");
  // } else {
  //   Serial.println("Internal thermometer disconnected");
  //   intTemp = -1;
  //   appendFile(SD, logfile, intTemp);
  //   appendFile(SD, logfile, ", ");
  // }

  //fetch BME688 data
	bme68xData data;

	bme.setOpMode(BME68X_FORCED_MODE);
	delay(500+bme.getMeasDur()/200);

	if (bme.fetchData())
	{
		bme.getData(data);
		Serial.print(String(millis()) + ", ");
		Serial.print(String(data.temperature) + ", ");
		Serial.print(String(data.pressure) + ", ");
		Serial.print(String(data.humidity) + ", ");
		Serial.print(String(data.gas_resistance) + ", ");
		Serial.println(data.status, HEX);

    appendFile(SD, logfile, data.temperature);
    appendFile(SD, logfile, ", ");
    appendFile(SD, logfile, data.pressure);
    appendFile(SD, logfile, ", ");
    appendFile(SD, logfile, data.humidity);
    appendFile(SD, logfile, ", ");
    appendFile(SD, logfile, data.gas_resistance);
    appendFile(SD, logfile, ", ");
    appendFile(SD, logfile, data.status);
	}

  // uint32_t data_uv = 0;
  // data_uv= ltr390.readOriginalData();//Get UV raw data
  // Serial.print("data:");
  // Serial.println(data_uv);
  // delay(1000);
  // appendFile(SD, logfile, ", ");
  // appendFile(SD, logfile, data_uv);

  appendFile(SD, logfile, "\n");

  
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

