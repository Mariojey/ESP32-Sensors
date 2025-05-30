#include <Arduino.h>
#include <SPI.h>
#include "FS.h"
#include <SD.h>
#include <Wire.h>
#include <ctime>
#include <numeric>
#include <iostream>
#include <BME280I2C.h>



//SD Pinout
#define SD_CS_PIN 5

//ADXL356 Pinout
#define XY_PIN 34
#define Z_PIN 35

//Piezo Pinout
#define piezoPin 33

//Control Pinout
#define LED_PIN 4


//ADXL356 Calibration Global Values
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


//For Mutex
struct EnvData {
  float temp = NAN;
  float press = NAN;
  float hum = NAN;
  uint8_t status = NAN;
  uint32_t lastUpdate = 0;
};


EnvData envData;

//Logging global variables
File logFile;
char logFileName[32];

//For Mutex
SemaphoreHandle_t dataMutex;

//SD Controll
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

void writeFile(fs::FS &fs, const char * path, const char * message);

//BME280
BME280I2C bme;

void envTask(void *pvParameters){
  while(1){

    float temp(NAN), hum(NAN), pres(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pres, temp, hum, tempUnit, presUnit);
    
    Serial.print(pres + ", ");
    Serial.print(temp + ", ");
    Serial.print(hum + ", ");

    if(xSemaphoreTake(dataMutex, portMAX_DELAY)){
      envData.temp = temp;
      envData.press = pres;
      envData.hum = hum;
      xSemaphoreGive(dataMutex);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void motionTask(void *pvParameters){
  while(1){

  if(xSemaphoreTake(dataMutex, portMAX_DELAY)){

    appendFile(SD, logFileName, envData.temp);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, envData.press);
    appendFile(SD, logFileName, ", ");
    appendFile(SD, logFileName, envData.hum);
    appendFile(SD, logFileName, ", ");

    xSemaphoreGive(dataMutex);

  }

      //ADXL356C
  int16_t val_xy = analogRead(XY_PIN);
  int16_t val_z = analogRead(Z_PIN);

  val_xy -= cali_data_xy;
  val_z -= cali_data_z;
  
  Serial.print(",");
  Serial.print(val_xy* scale / 1000.0);
  Serial.print(",");
  Serial.print(val_z * scale / 1000.0);

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
  Serial.println(", ");
  Serial.println(vib);
  char vib_buffer[32];
  sprintf(vib_buffer, ",%d\n",vib);
  appendFile(SD, logFileName, vib_buffer);



    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void setup() {
    Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  Wire.begin();

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

  sprintf(logFileName, "/balon_%d.txt", fileCount);


  writeFile(SD, logFileName, "Start Log");
  appendFile(SD, logFileName, "\n");

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(500);
  }
  //BME Onboarding
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);


  xTaskCreatePinnedToCore(envTask, "Env Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(motionTask, "Motion Task", 4096, NULL, 1, NULL, 1);

}

void loop() {
  // put your main code here, to run repeatedly:

}
