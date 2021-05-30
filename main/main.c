/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "protocol_examples_common.h"
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "driver/touch_sensor_common.h"
#include "driver/ledc.h"
#include "mdns.h"
#include "esp_sntp.h"

#define LEDC_FADE_TIME    (3000)
#define BLINK_GPIO 5

static const char *TAG = "example";
TaskHandle_t ledToggleTaskHandle = NULL, logTaskHandle = NULL;
void ledToggleTask(void *);

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req) {
    static int led_status = 0;

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    ESP_LOGI("Hello Handler", "Led_status ist %d", led_status);
    if (led_status == 1) {
        const char *resp_str = "The LED will now be toggled ON";
        httpd_resp_send(req, resp_str, strlen(resp_str));
        xTaskCreatePinnedToCore(ledToggleTask, "ledToggleTask", 4096, (void *) 1, 10, &ledToggleTaskHandle, 1);
    } else {
        const char *resp_str = "The LED will now be toggled OFF";
        httpd_resp_send(req, resp_str, strlen(resp_str));
        xTaskCreatePinnedToCore(ledToggleTask, "ledToggleTask", 4096, (void *) 0, 10, &ledToggleTaskHandle, 1);
    }
    led_status = !led_status;

    return ESP_OK;
}

static const httpd_uri_t hello = {
        .uri       = "/hello",
        .method    = HTTP_GET,
        .handler   = hello_get_handler,
        /* Let's pass response string in user
         * context to demonstrate it's usage */
        .user_ctx  = "Hello World!"
};

static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    int duty;
    sscanf(&buf, "%d", &duty);

    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 5000,                      // frequency of PWM signal
            .speed_mode = LEDC_LOW_SPEED_MODE,   // timer mode
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

    switch (duty) {
        case 0:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0*444);
            break;
        case 1:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 1*444);
            break;
        case 2:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 2*444);
            break;
        case 3:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 3*444);
            break;
        case 4:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 4*444);
            break;
        case 5:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 5*444);
            break;
        case 6:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 6*444);
            break;
        case 7:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 7*444);
            break;
        case 8:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 8*444);
            break;
        case 9:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 9*444);
            break;
        default:
            ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0*444);
            ESP_LOGE("Switch case", "wrong argument");
            break;
    }
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);



    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
static const httpd_uri_t ctrl = {
        .uri       = "/ctrl",
        .method    = HTTP_PUT,
        .handler   = ctrl_put_handler,
        .user_ctx  = NULL
};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handler
        ESP_LOGI(TAG, "Registering URI handler");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &ctrl);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server) {
    // Stop the httpd server
    httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    httpd_handle_t *server = (httpd_handle_t *) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data) {
    httpd_handle_t *server = (httpd_handle_t *) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void ledToggleTask(void *pvParameters) {
    int val = (int) pvParameters;
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 5000,                      // frequency of PWM signal
            .speed_mode = LEDC_LOW_SPEED_MODE,   // timer mode
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

    if (val == 1) {
        ledc_set_fade_with_time(ledc_channel.speed_mode,
                                ledc_channel.channel,
                                4000,
                                LEDC_FADE_TIME);
    } else {
        ledc_set_fade_with_time(ledc_channel.speed_mode,
                                ledc_channel.channel,
                                0,
                                LEDC_FADE_TIME);
    }
    ledc_fade_start(ledc_channel.speed_mode,
                    ledc_channel.channel,
                    LEDC_FADE_NO_WAIT);
    vTaskDelete(NULL);
}

void start_mdns_service()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set("ghp-esp");
    //set default instance
    mdns_instance_name_set("esp rest-api example");
}

void logTask() {
    time_t T = time(NULL);
    struct tm tm = *localtime(&T);
    while (1) {
        T = time(NULL);
        tm = *localtime(&T);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        ESP_LOGI("logTask", "Current Time is: %02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
}

void app_main(void) {
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    start_mdns_service();

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    setenv("TZ", "CST-2", 1);
    tzset();

    server = start_webserver();

    xTaskCreatePinnedToCore(logTask, "logTask", 4096, (void*)1, 10, &logTaskHandle, 0);
}
