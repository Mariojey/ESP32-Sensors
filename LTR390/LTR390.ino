/*
  CODE FOR TESTS IN UV COMPARTMENT
*/

#include "Arduino.h"

//LTR390
#include "DFRobot_LTR390UV.h"
DFRobot_LTR390UV ltr390(LTR390UV_DEVICE_ADDR, &Wire);


//SD
#include "FS.h"
#include "SD.h"
#include "SPI.h"

void listDir(fs::FS &fs, const char *dirname, uint8_t levels){

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open direcotry!");
    return;
  }

  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();

  while(file){

    if(file.isDirectory()){
      Serial.print("  DIR:  ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.path(), levels -1);
      }
    }else{
      Serial.print(" FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %lu ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
  file.close();
}



byte uv_data[2];
float uv;
float UVindex;

void setup() {
  //Init I2C mode
  Wire.begin();
  Serial.begin(115200);

  //Init SPI for SD Card
#ifdef REASSIGN_PINS
  SPI.begin(sck,miso,mosi,cs);
  if (!SD.begin(cs)){
#else
  if(!SD.begin()){
#endif
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  appendFile(SD, "/ltr390_17_04_10_06.txt","UV-2\n");

  while(ltr390.begin() != 0){
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
  char uv_buffer[32];
  sprintf(uv_buffer, "%d\n",data_uv);
  appendFile(SD, "/ltr390_17_04_10_06.txt", uv_buffer);
  delay(1000);

}
