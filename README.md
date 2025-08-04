<!-- HEADING -->

# Project Technical Documentation

## Table of Content
1. [Development Environment](#development-environment)
2. [Sensors](#sensors)
    1. [UV](#uv-sensor---ltr390)
    2. [Temp, Hum, Pres](#temperature-humidity-and-pressure-sensor-bme688)
    3. [Temp, Hum, Pres](#temperature-humidity-and-pressure-sensor-bme280)
3. [Data Storage](#data-storage)

## Development Environment

### IDE

**-> [Arduino IDE](https://www.arduino.cc/en/software/) v2.3.6 with library manager**

### ESP32 Board

**-> ESP32 DevKitC V1: ESP32 WiFi + BT 4.2 - with ESP-WROOM-32 [>>>LINK<<<](https://botland.com.pl/esp32/8893-esp32-wifi-bt-42-platforma-z-modulem-esp-wroom-32-zgodny-z-esp32-devkit-5904422337438.html?cd=18298825651&ad=&kd=&gad_campaignid=17764098689)**

### Full Code

**[Version with RTOS](./StratosWithRTOS_345_280/StratosWithRTOS_345_280.ino)** <- link

## Sensors

### UV Sensor - LTR390

*A sensor manufactured by DFRobot featuring the LTR390 chip, enabling the measurement of ultraviolet radiation.*

>**Required Libraries to Install:**<br /> 
>*DFRobot_LTR390UV*

***You need to locate the main Sketchbook directory. To do this, open the Arduino IDE, go to File → Preferences, and copy the path to the Sketchbook.
Next, download the sensor library from the provided [link](https://github.com/DFRobot/DFRobot_LTR390UV) and extract the DFRobot_LTR390UV-master folder into the libraries directory located inside your Sketchbook folder.
To ensure that the library was added correctly, restart the IDE and go to File → Sketchbook — the library should appear in the dropdown list.***

**You must switch the sensor to I2C mode (there is an I2C/UART selector switch on the sensor).**

**Pinout:**
<br />
| Sensor | ESP32 |
|--------|-------|
|+|3V3|
|-|GND|
|D/T|D21|
|C/R|D22|

**Example:**
```c++
#include "Arduino.h"
#include "DFRobot_LTR390UV.h"

DFRobot_LTR390UV ltr390(LTR390UV_DEVICE_ADDR, &Wire);

void setup(){

    Wire.begin(); //Initialization I2C communication
    Serial.begin(115200);

    while(ltr390.begin() != 0){ //
        Serial.println("Sensor initialize failed!");
        delay(1000);
    }

    Serial.println("Sensor initialize success!!");
    ltr390.setALSOrUVSMeasRate(ltr390.e18bit, ltr390.e100ms);
    ltr390.setALSOrUVSGain(ltr390.eGain3);
    ltr390.setMode(ltr390.eUVSMode);
}

void loop() {
  
    uint32_t data_uv = 0;
    data_uv = ltr390.readOriginalData();
    Serial.print("UV:");
    Serial.println(data_uv);
    delay(1000); //Delay before next measurement in milliseconds

}
```
### Temperature, Humidity, and Pressure Sensor BME688

*The sensor supports I2C and SPI communication interfaces.*

>**Required Libraries to Install:**<br /> 
>*BME68x Sensor library*

***This library can be installed directly via the Library Manager.
Find the Library Manager on the left sidebar of Arduino IDE, enter the name of the library, and install it.***

**Pinout:**
<br />
| Sensor | ESP32 |
|--------|-------|
|VCC|3V3|
|GND|GND|
|SDA/MOSI|D21|
|SCL/SCK|D22|
|*ADDR/MISO|GND(or VCC)|

*only if you need to change default I2C address

**Example:**
```c++
    #include "Arduino.h"
    #include "bme68xLibrary.h"
    #include "Wire.h"

    #define BME_I2C 0x77 //or 0x76 if ADDR is connected to GND

    Bme68x bme;

    void setup(){
        
        Serial.begin(115200);        
        Wire.begin();
        bme.begin(BME_I2C, Wire);     //I2C mode

        if (bme.checkStatus() == BME68X_ERROR)
		{
			Serial.println("Sensor error:" + bme.statusString());
			return;
		}
		else if (bme.checkStatus() == BME68X_WARNING)
		{
			Serial.println("Sensor Warning:" + bme.statusString());
		}

        /* Set the default configuration for temperature, pressure and humidity */
	    bme.setTPH();

	        /* Set the heater configuration to 300 deg C for 100ms for Forced mode */
	    bme.setHeaterProf(300, 100);

        bme.setOpMode(BME68X_FORCED_MODE);
	    
    }

    void loop(){

        bme68xData data;

        bme.setOpMode(BME68X_FORCED_MODE);
	    delay(bme.getMeasDur()/200);

        if(bme.fetchData()){
            bme.getData(data);
            Serial.println(data.status, HEX);
		    Serial.print(String(data.temperature) + ", "); //Celsius deg
		    Serial.print(String(data.pressure) + ", "); //Pascal deg
		    Serial.print(String(data.humidity) + ", "); //1-100 %
		    Serial.print(String(data.gas_resistance) + ", ");
        }else{
            Serial.print("Cannot read data!");
        }
    }

```

### Temperature, Humidity, and Pressure Sensor BME280

>**Required Libraries to Install:**<br /> 
>*BME280I2C*


***This library can be installed directly via the Library Manager.
Find the Library Manager on the left sidebar of Arduino IDE, enter the name of the library, and install it.***

**Pinout:**
<br />
| Sensor | ESP32 |
|--------|-------|
|VCC|3V3|
|GND|GND|
|SDA/MOSI|D21|
|SCL/SCK|D22|
|*ADDR/MISO|GND|

*only if you need to change default I2C address

**Example:**
```c++
#include <Arduino.h>
#include <Wire.h>
#include <BME280I2C.h>

BME280I2C bme;

void setup(){

    Wire.begin();
    Serial.begin(115200);

    while(!bme.begin()){
        Serial.println("Could not find BME280 sensor!");
        delay(500);
    }
}

void loop(){

    float temp(NAN), hum(NAN), pres(NAN);
    
    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pres, temp, hum, tempUnit, presUnit);
    
    Serial.print(pres);
    Serial.print(",");
    Serial.print(temp);
    Serial.print(",");
    Serial.println(hum);

    delay(100);
}
```

### Links

|Sensor|Link|
|------|----|
|**UV**|**[Gravity - LTR390 DFRobot SEN0540](https://botland.com.pl/gravity-czujnik-swiatla-i-koloru/22672-gravity-czujnik-swiatla-ultrafioletowego-uv-ltr390-uv-01-i2cuart-dfrobot-sen0540-6959420923175.html?cd=20567593583&ad=&kd=&gad_campaignid=20571183016)**|
|**Env**|**[Bosch BME688](https://botland.com.pl/czujniki-multifunkcyjne/23394-bme688-czujnik-srodowiskowy-4w1-modul-ai-i2c-spi-waveshare-24244.html?cd=20567593583&ad=&kd=&gad_campaignid=20571183016)**|
|**Env**|**[Bosch BME280](https://botland.com.pl/czujniki-multifunkcyjne/13463-bme280-czujnik-wilgotnosci-temperatury-oraz-cisnienia-i2cspi-33v5v-waveshare-15231-5904422377304.html?cd=20567593583&ad=&kd=&gad_campaignid=20571183016)**|

## Data Storage

### SD Adapter

**[Adapter with SPI interface](https://botland.com.pl/akcesoria-do-kart-pamieci/1507-modul-czytnika-kart-sd-5903351241342.html?cd=22830046247&ad=&kd=&gad_campaignid=22833845461)**

**Pinout:**
<br />
| Module | ESP32 |
|--------|-------|
|GND|GND|
|+3.3|3V3|
|+5||
|CS|D5|
|MOSI|D23|
|SCK|D18|
|MISO|D19|
|GND||

### Code

* File system libraries
```c++
#include <Arduino.h>
#include <SPI.h>
#include "FS.h"
#include <SD.h>
```

*Below are file handling functions to be implemented globally (outside the setup and loop functions).*

* Writing to a File (Overwriting Previous Data)

```c++
void writeFile(fs::FS &fs, const char * path, const char * message){

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    // "Failed to open file for writing";
    return;
  }
  if(file.print(message)){
    // You can print information
  } else {
    // You can print information
  }
  file.close();
}
```

* Appending Data to a File

```c++
template <typename T>
void appendFile(fs::FS &fs, const char * path, T message){

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    // Serial.println("Failed to open file for appending");
    return;
  }
  if(!file.print(message)){
    // You can print information
  }
  file.close();
}
```

* Counting Files in a Selected Directory on the SD Card

```c++
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
```

**Example program that saves UV sensor measurement data to an SD card, using a function that determines the file name based on the number of existing files on the card.**
```c++
    //Including libraries for file system
    #include "DFRobot_LTR390UV.h"

    #ifndef PIN_CS
    #define PIN_CS 5
    #endif

    DFRobot_LTR390UV ltr390(LTR390UV_DEVICE_ADDR, &Wire);

    //Implementation previous functions: writeFile, appendFile, countFiles

    char logFileName[32];

    void setup(){
        Wire.begin(); //Initialization I2C communication
        Serial.begin(115200);

        while(ltr390.begin() != 0){ //
            Serial.println("Sensor initialize failed!");
            delay(1000);
        }

        Serial.println("Sensor initialize success!!");
        ltr390.setALSOrUVSMeasRate(ltr390.e18bit, ltr390.e100ms);
        ltr390.setALSOrUVSGain(ltr390.eGain3);
        ltr390.setMode(ltr390.eUVSMode);

        if(!SD.begin(PIN_CS)){
            Serial.println("Card Mount Failed");
            return;
        }

        int fileCount = countFiles(SD, "/");

        sprintf(logFileName, "/logs_nr_%d.txt", fileCount); 

        //Creating file
        writeFile(SD, logFileName, "Start Log");
        appendFile(SD, logFileName, "\n");
    }

    void loop(){
        uint32_t data_uv = 0;
        data_uv = ltr390.readOriginalData();
        Serial.print("UV:");
        Serial.println(data_uv);
        char uv_buffer[32];
        sprintf(uv_buffer, "%d\n",data_uv);
        appendFile(SD, logFileName, uv_buffer);
        delay(500);
    }


```

## Sensor Manufacturers Copyrights

 * Copyright (C) 2021 Bosch Sensortec GmbH
 * SPDX-License-Identifier: BSD-3-Clause






