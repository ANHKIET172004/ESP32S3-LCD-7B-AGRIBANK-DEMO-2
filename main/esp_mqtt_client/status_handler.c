#include "status_handler.h"

static const char *TAG = "MQTT_BACKUP"; 

void save_status(const char *status)
{
    if (status == NULL) {
        ESP_LOGE(TAG, "Invalid status");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_STATUS", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }


    err = nvs_set_str(nvs_handle, "status", status);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Save status successfully ");
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed:%s ", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}



esp_err_t read_status(char *status, size_t max_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_STATUS", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "status", NULL, &required_size);// number
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get size for number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    if (required_size > max_len) {
        ESP_LOGE(TAG, "Buffer too small: required %d bytes, provided %d", required_size, max_len);
        nvs_close(nvs_handle);
        return ESP_ERR_INVALID_SIZE;
    }

    err = nvs_get_str(nvs_handle, "status", status, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read status: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Read status: %s", status);
    nvs_close(nvs_handle);
    return ESP_OK;
}
