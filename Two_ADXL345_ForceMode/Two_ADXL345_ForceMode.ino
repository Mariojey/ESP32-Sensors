#include <Wire.h>

TwoWire I2C_ADXL2 = TwoWire(1);

#define ADXL1_ADDR 0x1D  // Czujnik 1 na Wire
#define ADXL2_ADDR 0x53  // Czujnik 2 na Wire1

int8_t acc1_raw[6];
int8_t acc2_raw[6];

bool isFirst = true;

void setupADXL(TwoWire &bus, uint8_t address) {
  bus.beginTransmission(address);
  bus.write(0x2D);  // Power Control
  bus.write(0x08);  // Measurement mode
  bus.endTransmission();

  bus.beginTransmission(address);
  bus.write(0x31);  // Data format
  bus.write(0x0B);  // +-16g
  bus.endTransmission();
}

int16_t* readADXL(TwoWire &bus, uint8_t address, int8_t* raw_data) {
  static int16_t acc[3];

  bus.beginTransmission(address);
  bus.write(0x32);  // Data register
  if (bus.endTransmission(false) == 0) {
    bus.requestFrom(address, (uint8_t)6);
    for (int i = 0; i < 6; i++) {
      raw_data[i] = bus.read();
    }

    acc[0] = (int16_t)(raw_data[1] << 8 | raw_data[0]);
    acc[1] = (int16_t)(raw_data[3] << 8 | raw_data[2]);
    acc[2] = (int16_t)(raw_data[5] << 8 | raw_data[4]);
  } else {
    acc[0] = acc[1] = acc[2] = 0;
  }

  return acc;
}

void setup() {
  Serial.begin(115200);

  // Czujnik 1
  Wire.begin(21, 22);      // GPIO21 = SDA, GPIO22 = SCL
  setupADXL(Wire, ADXL1_ADDR);

  // Czujnik 2
  I2C_ADXL2.begin(17, 16); // GPIO17 = SDA, GPIO16 = SCL
  setupADXL(I2C_ADXL2, ADXL2_ADDR);
}

void loop() {
  if(isFirst){
    int16_t* acc1 = readADXL(Wire, ADXL1_ADDR, acc1_raw);
    delay(50);
    // int16_t* acc2 = readADXL(I2C_ADXL2, ADXL2_ADDR, acc2_raw);

    Serial.print("[ADXL1 - 0x1D] X="); Serial.print(acc1[0] * 0.0039);
    Serial.print(" Y="); Serial.print(acc1[1] * 0.0039);
    Serial.print(" Z="); Serial.println(acc1[2] * 0.0039);

    // Serial.print("[ADXL2 - 0x53] X="); Serial.print(acc2[0] * 0.0039);
    // Serial.print(" Y="); Serial.print(acc2[1] * 0.0039);
    // Serial.print(" Z="); Serial.println(acc2[2] * 0.0039);

    Serial.println("-----------------------------");
  }else{
      // int16_t* acc1 = readADXL(Wire, ADXL1_ADDR, acc1_raw);
    delay(50);
    int16_t* acc2 = readADXL(I2C_ADXL2, ADXL2_ADDR, acc2_raw);

    // Serial.print("[ADXL1 - 0x1D] X="); Serial.print(acc1[0] * 0.0039);
    // Serial.print(" Y="); Serial.print(acc1[1] * 0.0039);
    // Serial.print(" Z="); Serial.println(acc1[2] * 0.0039);

    Serial.print("[ADXL2 - 0x53] X="); Serial.print(acc2[0] * 0.0039);
    Serial.print(" Y="); Serial.print(acc2[1] * 0.0039);
    Serial.print(" Z="); Serial.println(acc2[2] * 0.0039);

    Serial.println("-----------------------------");
  }
  isFirst = !isFirst;
}
