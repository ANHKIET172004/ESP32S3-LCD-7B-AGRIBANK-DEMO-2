#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "mqtt_client.h"
#include "esp_mqtt_client/esp_mqtt_client.h"
#include "cJSON.h"

#include "rgb_lcd_port.h"
#include "gt911.h"
#include "lvgl_port.h"

#include "ui.h"

#include "wifi.h"

static const char *TAG = "MAIN";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t tp_handle = NULL;

uint8_t start_cnt=0;

SemaphoreHandle_t wifi_cred_mutex = NULL;

typedef struct {
    char topic[64];
    char data[512];
} mqtt_message_t;

QueueHandle_t mqtt_queue=NULL;

extern void mqtt_process_task(void *pvParameters);

extern void wifi_mqtt_manager_task(void *pv);


void app_main(void) {

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    tp_handle = touch_gt911_init();

    panel_handle = waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();

    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));


    if (lvgl_port_lock(-1)) {

        ui_init();
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "create main UI successfully");

    mqtt_queue = xQueueCreate(10, sizeof(mqtt_message_t));

    if (wifi_cred_mutex == NULL) {
    wifi_cred_mutex = xSemaphoreCreateMutex();
    if (wifi_cred_mutex == NULL) {
        ESP_LOGE("WIFI", "Failed to create wifi_cred_mutex");
    }
}

    //xTaskCreate(mqtt_process_task, "mqtt_process_task", 4096, NULL, 5, NULL);
    xTaskCreatePinnedToCore(mqtt_process_task,  "mqtt_retry",  4096, NULL, 5, NULL, 1 );

    //wifi_init_sta();
     
    xTaskCreatePinnedToCore(wifi_task, "wifi_task", 7* 1024, NULL, 6, &wifi_TaskHandle, 0);

  //  mqtt_start();
    xTaskCreatePinnedToCore(wifi_mqtt_manager_task, "wifi_mqtt_manager_task", 4* 1024, NULL, 4, &wifi_TaskHandle, 1);
    

    
}