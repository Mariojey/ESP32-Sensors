#include <Arduino.h>

struct Buffer{
  int iter = 1;
};

Buffer buffer;

SemaphoreHandle_t dataMutex;

void firstTask(void *pvParameters){
  int iter2 = 1;
  while(1){

    if(xSemaphoreTake(dataMutex, portMAX_DELAY)){
      Serial.print("Iter 1: ");
      Serial.print(buffer.iter);
      xSemaphoreGive(dataMutex);
    }
    Serial.print("Iter 2: ");
    Serial.println(iter2);
    iter2 += 1;

    vTaskDelay(50 / portTICK_PERIOD_MS);

  }
}

void secondTask(void *pvParameters){
  while(1){
    if(xSemaphoreTake(dataMutex, portMAX_DELAY)){
      buffer.iter = buffer.iter + 1;
      xSemaphoreGive(dataMutex);
    }

    vTaskDelay(400 / portTICK_PERIOD_MS);
  }
}

void setup() {

  Serial.begin(115200);

  dataMutex = xSemaphoreCreateMutex();

  if(dataMutex == NULL){
    Serial.println("Nie udało się utworzyć mutexu na buffor!");
    while(1);
  }

  delay(1000);

  xTaskCreatePinnedToCore(firstTask, "First", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(secondTask, "Second", 4096, NULL, 1, NULL, 1);

}

void loop() {
  // put your main code here, to run repeatedly:

}
