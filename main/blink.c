#include <limits.h>
/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/touch_sensor_common.h"
#include "driver/ledc.h"


TaskHandle_t logTaskHandle = NULL, readTaskHandle = NULL;
QueueHandle_t myQueue = NULL;

void readTask(void *pvParameters) {
    int gpioStatus = 0;
    srand(time(NULL));

    gpio_pad_select_gpio(GPIO_NUM_5);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_INPUT);

    while (1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpioStatus = gpio_get_level(GPIO_NUM_5);
        if (gpioStatus == 1){
            int r = rand();
            xQueueSend(myQueue, &r, 0 / portTICK_PERIOD_MS);
        }
    }
}

void logTask() {
    int receivedVal;
    while (1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        while (xQueueReceive(myQueue, &receivedVal, portMAX_DELAY)){
            ESP_LOGI("logTask", "Received queue element with value %d", receivedVal);
        }
    }
}

void app_main(void) {
    myQueue = xQueueCreate(10, sizeof(int));

    xTaskCreatePinnedToCore(readTask, "readTask", 4096, (void *)0, 10, &readTaskHandle, 0);
    xTaskCreatePinnedToCore(logTask, "logTask", 4096, (void*)1, 10, &logTaskHandle, 1);
}
