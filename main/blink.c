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

TaskHandle_t myTaskHandle1 = NULL, logTaskHandle = NULL;
QueueHandle_t myQueue = NULL;

void myTask1(void * pvParameters )
{
    int val = (int)pvParameters;
    ESP_LOGE("Task1", "My value is %d, setting GPIOs for blink and starting endless loop", val);
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        //printf("Turning off the LED Value: %d\n", val);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //printf("Turning on the LED\n");
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        int receivedVal;
        xQueueReceive(myQueue, &receivedVal, portMAX_DELAY);
        ESP_LOGW("Task1", "Received queue element with value %d", receivedVal);
    }
}

void myTask2(void * pvParameters )
{
    int val = (int)pvParameters;

    while(1) {
        val++;
        //ESP_LOGW("Task2", "My value %d, starting endless loop", val);
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

    touch_pad_init();
    touch_pad_config(0, 0);
    touch_pad_filter_start(0);
    uint16_t touchVal = 0;


// create task for this
    while (1){
        touch_pad_read(0, &touchVal);
        printf("Touchdata: %d\n", touchVal);
    }

    /*
    myQueue = xQueueCreate(1, sizeof(int));
    //xTaskCreatePinnedToCore(myTask1, "myTask1", 4096, (void*)42, 10, &myTaskHandle1, 0);
    xTaskCreatePinnedToCore(logTask, "logTask", 4096, (void*)1, 10, &logTaskHandle, 0);
    xTaskCreatePinnedToCore(myTask2, "myTask2", 4096, (void*)0, 10, &myTaskHandle1, 1);
*/
    ESP_LOGI("Main", "Ich bin am Ende der Main!");
}
