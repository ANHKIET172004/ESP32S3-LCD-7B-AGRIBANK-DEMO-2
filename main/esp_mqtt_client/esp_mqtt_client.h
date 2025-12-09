#ifndef ESP_MQTT_CLIENT_H
#define ESP_MQTT_CLIENT_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "esp_log.h"

void save_number(const char *number);
esp_err_t read_number(char *number, size_t max_len);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_start(void);

void save_status(const char *status);

esp_err_t read_status(char *status, size_t max_len);


#endif