#include "number_handler.h"

static const char *TAG = "MQTT_BACKUP"; 

void save_called_number(const char *number) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("number_cred", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle (%s)!", esp_err_to_name(err));
        return;
    }
    nvs_set_str(nvs_handle, "current_number", number);
    err = nvs_commit(nvs_handle);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "current number saved successfully!");
    } else {
        ESP_LOGE(TAG, "Failed to save current number: %s", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
}




void save_number(const char *number)
{
    if (number == NULL) {
        ESP_LOGE(TAG, "Invalid number");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Can't open NVS: %s", esp_err_to_name(err));
        return;
    }

    char temp[16];
    size_t required_size = sizeof(temp);
    err = nvs_get_str(nvs_handle, "current_number", temp, &required_size);
    const char *key_to_use = (err == ESP_OK) ? "next_number" : "current_number";

    err = nvs_set_str(nvs_handle, key_to_use, number);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Save %s successfully %s", number, key_to_use);
        } else {
            ESP_LOGE(TAG, "Commit failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Save failed %s: %s", key_to_use, esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}



esp_err_t read_number(char *number, size_t max_len)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_NUMBER", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "current_number", NULL, &required_size);// number
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

    err = nvs_get_str(nvs_handle, "number", number, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read number: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "Read number: %s", number);
    nvs_close(nvs_handle);
    return ESP_OK;
}
