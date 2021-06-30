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
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/touch_sensor_common.h"
#include "driver/ledc.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define LEDC_FADE_TIME    (3000)
/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define WARN_SECONDS CONFIG_WARN_SECONDS

TaskHandle_t messageQueueHandle = NULL, logTaskHandle = NULL, touchTaskHandle = NULL;
QueueHandle_t myQueue = NULL;

void touchTask(void *pvParameters) {
    int val = (int) pvParameters;
    int status = 0;
    int lastStatus = 0;
    touch_pad_init();
    touch_pad_config(0, 0);
    uint16_t touchVal = 0;
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 5000,                      // frequency of PWM signal
            .speed_mode = LEDC_LOW_SPEED_MODE,    // timer mode
            .timer_num = LEDC_TIMER_1,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 0,
            .gpio_num   = BLINK_GPIO,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_1,
    };
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

    while (1) {
        touch_pad_read(0, &touchVal);
        if (touchVal <= 300) {
            ledc_set_fade_with_time(ledc_channel.speed_mode,
                                    ledc_channel.channel,
                                    4000,
                                    LEDC_FADE_TIME);
            ledc_fade_start(ledc_channel.speed_mode,
                            ledc_channel.channel,
                            LEDC_FADE_NO_WAIT);
            status = 1;
        } else {
            ledc_set_fade_with_time(ledc_channel.speed_mode,
                                    ledc_channel.channel, 0, LEDC_FADE_TIME);
            ledc_fade_start(ledc_channel.speed_mode,
                            ledc_channel.channel, LEDC_FADE_NO_WAIT);
            status = 0;
        }
        if (status != lastStatus){
            lastStatus = status;
            vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
        }
        //vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void messageQueueTask(void *pvParameters) {
    int val = (int) pvParameters;
    while (1) {
        val++;
        xQueueSend(myQueue, &val, 1000 / portTICK_PERIOD_MS);
    }
}

void logTask() {
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

void app_main(void) {
    ESP_LOGI("Main", "Start of Main!");
    myQueue = xQueueCreate(1, sizeof(int));
    xTaskCreatePinnedToCore(touchTask, "touchTask", 4096, (void *) 0, 10, &touchTaskHandle, 0);
    xTaskCreatePinnedToCore(logTask, "logTask", 4096, (void*)1, 10, &logTaskHandle, 0);
    xTaskCreatePinnedToCore(messageQueueTask, "messageQueueTask", 4096, (void*)0, 10, &messageQueueHandle, 1);
    ESP_LOGI("Main", "End of Main!");
}

#pragma clang diagnostic pop