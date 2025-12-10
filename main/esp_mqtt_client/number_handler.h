#ifndef NUMBER_HANDLER_H
#define NUMBER_HANDLER_H


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
#include "lvgl.h"
#include "lvgl_port.h"
#include "ui.h"
#include "mqtt_data.h"

void save_called_number(const char *number) ;
void save_number(const char *number);
esp_err_t read_number(char *number, size_t max_len);

#endif