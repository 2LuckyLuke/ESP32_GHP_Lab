#include <limits.h>
/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/touch_sensor_common.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define WARN_SECONDS CONFIG_WARN_SECONDS

TaskHandle_t messageQueueHandle = NULL, logTaskHandle = NULL, touchTaskHandle = NULL;
QueueHandle_t myQueue = NULL;

void touchTask(void * pvParameters ){
    int val = (int)pvParameters;
    touch_pad_init();
    touch_pad_config(0, 0);
    //touch_pad_filter_start(0);
    uint16_t touchVal = 0;
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);


    while(1) {
        touch_pad_read(0, &touchVal);
        //printf("Touchdata: %d\n", touchVal);
        if (touchVal <= 300){
            gpio_set_level(BLINK_GPIO, 1);
        } else{
            gpio_set_level(BLINK_GPIO, 0);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void messageQueueTask(void * pvParameters ){
    int val = (int)pvParameters;
    while(1) {
        val++;
        xQueueSend(myQueue, &val, 1000/ portTICK_PERIOD_MS);
    }
}

void logTask(){
    time_t T = time(NULL);
    struct tm tm = *localtime(&T);
    int val = 1;
    int receivedVal;
    while (1) {
        T = time(NULL);
        tm = *localtime(&T);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI("logTask", "Time passed since boot: %02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        if (val % WARN_SECONDS == 0) {
            ESP_LOGW("logTask", "%d Seconds have passed", WARN_SECONDS);
            xQueueReceive(myQueue, &receivedVal, portMAX_DELAY);
            ESP_LOGW("logTask", "Received queue element with value %d", receivedVal);
        }
        val++;
    }
}

void app_main(void)
{
    ESP_LOGI("Main", "Ich bin grade in der Main!");

    myQueue = xQueueCreate(1, sizeof(int));
    xTaskCreatePinnedToCore(touchTask, "touchTask", 4096, (void*)0, 10, &touchTaskHandle, 0);
    //xTaskCreatePinnedToCore(logTask, "logTask", 4096, (void*)1, 10, &logTaskHandle, 0);
    //xTaskCreatePinnedToCore(messageQueueTask, "messageQueueTask", 4096, (void*)0, 10, &messageQueueHandle, 1);

    ESP_LOGI("Main", "Ich bin am Ende der Main!");
}
