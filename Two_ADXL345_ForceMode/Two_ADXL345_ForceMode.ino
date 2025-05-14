#include "Arduino.h"
#include <Wire.h>

#define ADXL1 0x53
#define ADXL2 0x1D

int8_t accelerations_1_raw[6]; //przyspieszenia x, y, z
int8_t accelerations_2_raw[6]; //przyspieszenia x, y, z


template <typename T>
int16_t * readADXL(uint8_t address, int8_t * accelertations_raw);

void setup(void) {
  // put your setup code here, to run once:

  Wire.begin();     //I2C mode
	Serial.begin(115200);

    //ADXL1 setup
  Serial.print("SETUP");
  Wire.beginTransmission(ADXL1);
  Wire.write(0x32); //rejestr zakresu danych
  Wire.write(0x0B); //+- 16g, powinno wystarczyć
  Wire.endTransmission();

  Wire.beginTransmission(ADXL1);
  Wire.write(0x2D); //Power ctl
  Wire.write(0x08); //Tryb pomiaru ciągłego
  Wire.endTransmission();

      //ADXL2 setup
  Serial.print("SETUP");
  Wire.beginTransmission(ADXL2);
  Wire.write(0x32); //rejestr zakresu danych
  Wire.write(0x0B); //+- 16g, powinno wystarczyć
  Wire.endTransmission();

  Wire.beginTransmission(ADXL2);
  Wire.write(0x2D); //Power ctl
  Wire.write(0x08); //Tryb pomiaru ciągłego
  Wire.endTransmission();
}

void loop() {
  // put your main code here, to run repeatedly:

    //fetch ADXL data
  int16_t * fromADXL1_ptr;
  int16_t * fromADXL2_ptr;

  fromADXL1_ptr = readADXL(ADXL1, accelerations_1_raw);
  fromADXL2_ptr = readADXL(ADXL2, accelerations_2_raw);

    Serial.print("Acceleration (x, y, z): ");
   Serial.print( "Xraw = ");
   Serial.print(fromADXL1_ptr[0] * 0.031);
   Serial.print( "Yraw = ");
   Serial.print(fromADXL1_ptr[1] * 0.031);
   Serial.print( "Zraw = ");
   Serial.print(fromADXL1_ptr[2] * 0.031);
    Serial.print( "Xraw = ");
   Serial.print(fromADXL2_ptr[0] * 0.031);
   Serial.print( "Yraw = ");
   Serial.print(fromADXL2_ptr[1] * 0.031);
   Serial.print( "Zraw = ");
   Serial.println(fromADXL2_ptr[2] * 0.031);

   delay(50);

}

int16_t *readADXL(uint8_t address, int8_t *accel_raw) {
  static int16_t acc[3];

  Wire.beginTransmission(address);
  Wire.write(0x32); // ADXL register start
  if (Wire.endTransmission() == 0) {
    Wire.requestFrom(address, (uint8_t)6);
    for (int i = 0; i < 6; i++) {
      accel_raw[i] = Wire.read();
    }

    acc[0] = accel_raw[0] | (accel_raw[1] << 8);
    acc[1] = accel_raw[2] | (accel_raw[3] << 8);
    acc[2] = accel_raw[4] | (accel_raw[5] << 8);
  } else {
    Serial.print("ADXL at 0x");
    Serial.print(address, HEX);
    Serial.println(" not responding!");
    acc[0] = acc[1] = acc[2] = 0;
  }

  return acc;
}

