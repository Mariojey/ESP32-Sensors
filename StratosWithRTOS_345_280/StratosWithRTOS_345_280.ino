#include <Arduino.h>
#include <SPI.h>
#include "FS.h"
#include <SD.h>
#include <Wire.h>
#include <ctime>
#include <numeric>
#include <iostream>
#include <BME280I2C.h>
#include "RTClib.h"




//SD Pinout
#define SD_CS_PIN 5

//ADXL356 Pinout
#define XY_PIN 34
#define Z_PIN 35

//Piezo Pinout
#define piezoPin 33

//Control Pinout
#define LED_PIN 4

//On Board Payload Pinout
#define SOUTH_PAYLOAD_STATS_IN 32
#define SOUTH_PAYLOAD_STATS_OUT 12

//Thor's board status
#define NORTH_PAYLOAD_STATS_OUT 13

//Rocket
#define  GPIO25_0 25
#define GPIO26_1 26
#define GPIO27_2 27
#define GPIO14_3 14


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

//South Payload
bool southPayloadStats = LOW;

//Init stats
bool bmeInit = false;
bool cardInit = false;
bool adxl356Init = false;
bool adxl345Init = false;
bool rtcInit = false;
bool piezoInit = false;

//For Mutex
struct EnvData {
  float temp = NAN;
  float press = NAN;
  float hum = NAN;
  uint8_t status = NAN;
  uint32_t lastUpdate = 0;
};


EnvData envData;

RTC_DS3231 rtc;

//--------------------SPIKE FOR TWO ADXL345 --NOT USING ATM------------------------------

// TwoWire I2C_ADXL2 = TwoWire(1);

// #define ADXL1_ADDR 0x1D  // Sensor 1 on Wire
// #define ADXL2_ADDR 0x53  // Sensor 2 on Wire1
// int8_t acc2_raw[6];

// bool isFirst = true;

//---------------------------------------------------------------------------------------

#define ADXL345 0x53

int8_t acc345_raw[6];

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
  adxl356Init = true;

  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(300);


  scale = (float)1000 / (cali_data_z - cali_data_xy);
  cali_data_z -= (float)980 / scale;

}

//------------FOR 2 ADXL345------------------
// void setupADXL(TwoWire &bus, uint8_t address) {
//   bus.beginTransmission(address);
//   bus.write(0x2D);  // Power Control
//   bus.write(0x08);  // Measurement mode
//   bus.endTransmission();

//   bus.beginTransmission(address);
//   bus.write(0x31);  // Data format
//   bus.write(0x0B);  // +-16g
//   bus.endTransmission();
// }
//------------------------------------------

int16_t* readADXL(int8_t * accelerations_raw) {
  static int16_t acc[3];

  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  if(!Wire.endTransmission()){

    Wire.requestFrom(ADXL345, 6);

    for (int i = 0; i < 6; i++){
      accelerations_raw[i] = Wire.end();
    }

    int16_t Xraw = accelerations_raw[0] | accelerations_raw[1] << 8; //move older bits by 8 and OR it with 
    int16_t Yraw = accelerations_raw[2] | accelerations_raw[3] << 8;
    int16_t Zraw = accelerations_raw[4] | accelerations_raw[5] << 8;
    acc[0] = Xraw;
    acc[1] = Yraw;
    acc[2] = Zraw;

    if(!adxl345Init){
      adxl345Init = true;
    }

    return acc;

  }else{

    Serial.println("ADXL disconnected");
    for(int i = 0; i < 6; i++){
      accelerations_raw[i] = -1;
    }

  }

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
    
    Serial.print(pres);
    Serial.print(",");
    Serial.print(temp);
    Serial.print(",");
    Serial.print(hum);
    Serial.print(",");

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

  DateTime now = rtc.now();

  char timestamp[32];

  sprintf(timestamp, "d%/%d/%d %d:%d:%d,",now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());
  appendFile(SD, logFileName, timestamp);

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


  // ADXL345
  int16_t * fromADXL_ptr;

  fromADXL_ptr = readADXL(acc345_raw);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[0] * 0.0031);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[1] * 0.0031);
  Serial.print(", ");
  Serial.print(fromADXL_ptr[2] * 0.0031);

  float x_g = fromADXL_ptr[0] * 0.0031;
  float y_g = fromADXL_ptr[1] * 0.0031;
  float z_g = fromADXL_ptr[2] * 0.0031;

  char adxl_345_buffer[32];
  sprintf(adxl_345_buffer, ",%.2f, %.2f, %.2f",x_g, y_g, z_g);
  appendFile(SD, logFileName, adxl_345_buffer);


//------------------------SPIKE WITH 2 ADXL345--------------------------------------
    // if(isFirst){
    //   int16_t* acc1 = readADXL(Wire, ADXL1_ADDR, acc1_raw);
    //   // int16_t* acc2 = readADXL(I2C_ADXL2, ADXL2_ADDR, acc2_raw);

    //   // Serial.print("[ADXL1 - 0x1D] X="); Serial.print(acc1[0] * 0.0039);
    //   // Serial.print(" Y="); Serial.print(acc1[1] * 0.0039);
    //   // Serial.print(" Z="); Serial.println(acc1[2] * 0.0039);

    //   float x_g = acc1[0] * 0.0039;
    //   float y_g = acc1[1] * 0.0039;
    //   float z_g = acc1[2] * 0.0039;

    //   char adxl_345_buffer[32];
    //   sprintf(adxl_345_buffer, ",%.2f, %.2f, %.2f",x_g, y_g, z_g);
    //   appendFile(SD, logFileName, adxl_345_buffer);
    // }else{
    //     // int16_t* acc1 = readADXL(Wire, ADXL1_ADDR, acc1_raw);
    //   int16_t* acc2 = readADXL(I2C_ADXL2, ADXL2_ADDR, acc2_raw);

    //   // Serial.print("[ADXL1 - 0x1D] X="); Serial.print(acc1[0] * 0.0039);
    //   // Serial.print(" Y="); Serial.print(acc1[1] * 0.0039);
    //   // Serial.print(" Z="); Serial.println(acc1[2] * 0.0039);

    //   // Serial.print("[ADXL2 - 0x53] X="); Serial.print(acc2[0] * 0.0039);
    //   // Serial.print(" Y="); Serial.print(acc2[1] * 0.0039);
    //   // Serial.print(" Z="); Serial.println(acc2[2] * 0.0039);

    //   // Serial.println("-----------------------------");

    //   float x_g = acc2[0] * 0.0039;
    //   float y_g = acc2[1] * 0.0039;
    //   float z_g = acc2[2] * 0.0039;

    //   char adxl_345_buffer[32];
    //   sprintf(adxl_345_buffer, ",%.2f, %.2f, %.2f",x_g, y_g, z_g);
    //   appendFile(SD, logFileName, adxl_345_buffer);
    // }
    // isFirst = !isFirst;

    //---------------------------------------------------------------------------



  //Piezo
  int vib = analogRead(piezoPin);
  Serial.println(", ");
  Serial.println(vib);
  char vib_buffer[32];
  sprintf(vib_buffer, ", %d,",vib);
  appendFile(SD, logFileName, vib_buffer);



    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void setup() {
    Serial.begin(115200);

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
    cardInit = true;
  }
  int fileCount = countFiles(SD, "/"); 

  sprintf(logFileName, "/lot_%d.txt", fileCount);


  writeFile(SD, logFileName, "Start Log");
  appendFile(SD, logFileName, "\n");
  delay(50);

  appendFile(SD, logFileName, "Go/No-Go Poll.....................................\n");
  appendFile(SD, logFileName, "RTC..........................................");

  if (! rtc.begin()) {
    appendFile(SD, logFileName, "ERROR\n");
  }

    if (rtc.lostPower()) {
    appendFile(SD, logFileName, "RTC lost power, set the time manually during data analysis!\n");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(1939, 9, 1, 4, 45, 0));
  }
  rtcInit = true;
  appendFile(SD, logFileName, "READY\n");
  appendFile(SD, logFileName, "BME..........................................");
  
  int bme_checking_iter = 0;
  while(!bme.begin())
  {
    if(bme_checking_iter <= 10){
      Serial.println("Could not find BME280 sensor!");
      appendFile(SD, logFileName, "READY\n");
      appendFile(SD, logFileName, "BME..........................................");
      delay(500);
    }else{
      bmeInit = true;
      break;
    }
    bme_checking_iter ++;
  }
  //BME Onboarding
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);

  appendFile(SD, logFileName, "READY\n");
  appendFile(SD, logFileName, "356..........................................");

  delay(100);
  analogReadResolution(12);
  calibration();
  delay(1700);

  appendFile(SD, logFileName, "READY\n");

  appendFile(SD, logFileName, "345..........................................");
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

  int16_t * fromADXL_ptr;

  fromADXL_ptr = readADXL(acc345_raw);

  if(adxl345Init){
    appendFile(SD, logFileName, "READY\n");
  }else{
    appendFile(SD, logFileName, "NINFO\n");
  }

  appendFile(SD, logFileName, "PIEZO........................................");
  
  int vib = analogRead(piezoPin);
  if(vib < 0){
    appendFile(SD, logFileName, "ERROR\n");
  }else{
    appendFile(SD, logFileName, "READY\n");
    piezoInit = true;
  }

  
  //ROCKET Communication
  pinMode(GPIO25_0, INPUT);
  pinMode(GPIO26_1, INPUT);
  pinMode(GPIO27_2, INPUT);
  pinMode(GPIO14_3, INPUT);

  //Thor's validation
  pinMode(NORTH_PAYLOAD_STATS_OUT, OUTPUT);

  //SOUTH Payload Communication
  pinMode(SOUTH_PAYLOAD_STATS_IN, INPUT);
  pinMode(SOUTH_PAYLOAD_STATS_OUT, OUTPUT);

  int south_check_iter = 0;
  while(south_check_iter < 50){

    southPayloadStats = digitalRead(SOUTH_PAYLOAD_STATS_IN);

    appendFile(SD, logFileName, "CONNECTION.EXP.2.............................");
  
    if(southPayloadStats == HIGH){
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
      digitalWrite(SOUTH_PAYLOAD_STATS_OUT, HIGH);
      appendFile(SD, logFileName, "OK\n");
      break;
    }
    
    char iter_buffer[32];
    sprintf(iter_buffer, " %d/n",south_check_iter);
    appendFile(SD, logFileName, iter_buffer);

    delay(100);
    south_check_iter++;
  }


  appendFile(SD, logFileName, "Timestamp, Temp, Press, Hum, ACC XY, ACC Z, ACC x, ACC y, ACC z, Vibration, GPIO25, GPIO26, GPIO27, GPIO14 \n");

  Serial.println("Check SD Card for logs!");

  xTaskCreatePinnedToCore(envTask, "Env Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(motionTask, "Motion Task", 4096, NULL, 1, NULL, 1);

}

void loop() {
  // put your main code here, to run repeatedly:

}
