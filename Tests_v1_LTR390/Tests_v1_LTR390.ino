#include "Arduino.h"
#include "DFRobot_LTR390UV.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

DFRobot_LTR390UV ltr390(LTR390UV_DEVICE_ADDR, &Wire);

// Plik logu (dynamicznie generowany)
char logFileName[32];

// Oblicz indeks UV w zakresie 0-11 (przybliżenie)
int mapUVIndex(uint32_t uv_raw) {
  // Skalowanie - dostosuj w zależności od typowego zakresu
  float scaled = (float)uv_raw / 1800.0 * 11.0;
  if (scaled > 11.0) scaled = 11.0;
  return (int)(scaled + 0.5);
}

// Zlicz pliki w katalogu głównym
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

void writeFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(message);
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  file.print(message);
  file.close();
}

void setup() {
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

  // if (!SD.begin()) {
  //   Serial.println("Card Mount Failed");
  //   return;
  // }

  int fileCount = countFiles(SD, "/");

  // Utwórz nazwę nowego pliku
  sprintf(logFileName, "/ltr390_24_%d.txt", fileCount);
  Serial.print("Nowy plik logu: ");
  Serial.println(logFileName);

  // Dodaj nagłówek do pliku
  writeFile(SD, logFileName, "UV_raw,UV_index\n");

  while (ltr390.begin() != 0) {
    Serial.println("Sensor initialize failed!");
    delay(1000);
  }

  Serial.println("Sensor initialize success!");
  ltr390.setALSOrUVSMeasRate(ltr390.e18bit, ltr390.e100ms);
  ltr390.setALSOrUVSGain(ltr390.eGain3);
  ltr390.setMode(ltr390.eUVSMode);
}

void loop() {
  uint32_t data_uv = ltr390.readOriginalData();
  int uv_index = mapUVIndex(data_uv);

  Serial.print("UV RAW: ");
  Serial.print(data_uv);
  Serial.print(" | UV Index: ");
  Serial.println(uv_index);

  char buffer[64];
  sprintf(buffer, "%lu,%d\n", data_uv, uv_index);
  appendFile(SD, logFileName, buffer);

  delay(1000);
}
