/*****************************************************************************
 * | File       :   main.c
 * | Author     :   Waveshare team
 * | Function   :   Main function
 * | Info       :
 * |                UI Design：
 *                          1. User Login and Creation: Users can log in or create new accounts, and the created users are saved to NVS, so data is not lost after power-down.
 *                          2. Wi-Fi: Can connect to Wi-Fi and start an access point (hotspot).
 *                          3. RS485: Can send and receive data, with data displayed on the screen.
 *                          4. PWM: Can modify PWM output in multiple ways to control screen brightness. Additionally, it can display information from a Micro SD card.
 *                          5. CAN: Can send and receive data, with data displayed on the screen.
 *----------------
 * | Version    :   V1.0
 * | Date       :   2025-05-08
 * | Info       :   Basic version
 *
 ******************************************************************************/
#include "rgb_lcd_port.h" // Header for Waveshare RGB LCD driver
#include "gt911.h"        // Header for touch screen operations (GT911)
#include "lvgl_port.h"    // Header for LVGL port initialization and locking
//#include "add_password.h" // Header for password handling logic
//#include "pwm.h"          // Header for PWM initialization (used for backlight control)
#include "wifi.h"         // Header for Wi-Fi functionality
//#include "sd_card.h"      // Header for SD card operations
//#include "D:/ESP32/ESP32S3_LCD_7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"           // Header for user interface initialization
#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/ui/ui.h"
#include "nvs_flash.h"

// các file header hỗ trợ gatt

#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_server/gatt_server.h"
//#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_client/gatt_client.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/event_groups.h"

// API Configuration
#define API_BASE_URL "http://10.10.1.46:12001/api"  
#define DEVICE_ID "5"
#define DEVICE_NAME2 "ESP32-SN100010"
#define DEVICE_SERIAL "SN100010"
#define DEVICE_MAC "00:1A:2B:3C:4D:10"

// Authentication
#define API_EMAIL "john@example.com"     
#define API_PASSWORD "password123"       
#define API_TOKEN "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJodHRwOi8vMTI3LjAuMC4xOjEyMDAxL2FwaS9sb2dpbiIsImlhdCI6MTc1OTIwODEyNywiZXhwIjoxNzU5Mjk0NTI3LCJuYmYiOjE3NTkyMDgxMjcsImp0aSI6IjdhSmhXMWtZMUFYT05rYnIiLCJzdWIiOiIxIiwicHJ2IjoiMjNiZDVjODk0OWY2MDBhZGIzOWU3MDFjNDAwODcyZGI3YTU5NzZmNyJ9.ZDfUOCN-PdhGfdb1ThwVbFfHBn_CGLYWZRGqHjUHmLA"                     
#define MAX_HTTP_OUTPUT_BUFFER 2048
char http_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
char auth_token[512] = {0};


static const char *TAG = "main"; // Tag used for ESP log output
static const char *API_TAG = "API_CLIENT";

static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

extern int rate;
extern int score;


 extern EventGroupHandle_t wifi_event_group;
extern int WIFI_CONNECTED_BIT ;

 extern int start_api;


/**
 * @brief Main application function.
 *
 * This function initializes the necessary hardware components such as the touch screen
 * and RGB LCD display, sets up the LVGL library for graphics rendering, and runs
 * the LVGL demo UI.
 *
 * - Initializes the GT911 touch screen controller.
 * - Initializes the Waveshare ESP32-S3 RGB LCD display.
 * - Initializes the LVGL library for graphics rendering.
 * - Runs the LVGL demo UI.
 *
 * @return None
 */


////////////gatt
int connect_success=0;
void mainscreen_wifi_rssi_task(void *pvParameters) {
    wifi_ap_record_t ap_info;
   
    while (1) {
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            connect_success=1;
            ESP_LOGI("RSSI", "Connected SSID:%s, BSSID:%02X:%02X:%02X:%02X:%02X:%02X, RSSI:%d dBm",
                     ap_info.ssid,
                     ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
                     ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5],
                     ap_info.rssi);


              if (ap_info.rssi == 0 && ap_info.ssid[0] == '\0')
        {    
            //if (lvgl_port_lock(0)) {
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );

            //lvgl_port_unlock();
           // }   
            //break;
            //vTaskDelete(NULL);
        }
         
         else if(ap_info.rssi > -25)  // Strong signal (RSSI > -25)
        {   
            lv_obj_clear_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with strong signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_4_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -25) && (ap_info.rssi > -50))  // Medium signal
        { 
            lv_obj_clear_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with medium signal icon
          //  WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_3_png, (const char *)ap_info[i].ssid);
        }
        else if ((ap_info.rssi < -50) && (ap_info.rssi > -75))  // Weak signal
        { 
            lv_obj_clear_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with weak signal icon
           // WIFI_List_Button = lv_list_add_btn(ui_WIFI_SCAN_List, &ui_img_wifi_2_png, (const char *)ap_info[i].ssid);
        }
        else  // Very weak signal (RSSI < -75)
        { 
            lv_obj_clear_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            //lvgl_port_unlock();
            // Add button with very weak signal icon
              //_ui_flag_modify(ui_WIFI_SCAN_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);      
   }         
        } else {
            ESP_LOGW("RSSI", "Not connected to any AP");
            connect_success=0;
            lv_obj_clear_flag(ui_Image20, LV_OBJ_FLAG_HIDDEN );  
            lv_obj_add_flag(ui_Image24, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image32, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image31, LV_OBJ_FLAG_HIDDEN );
            lv_obj_add_flag(ui_Image34, LV_OBJ_FLAG_HIDDEN );
        }
      
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay 2s
    }
}





//////////////////////////////

// ==================== HTTP Event Handler ====================
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                int copy_len = evt->data_len;
                if (copy_len + strlen(http_response_buffer) < MAX_HTTP_OUTPUT_BUFFER) {
                    strncat(http_response_buffer, (char*)evt->data, copy_len);
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

// ==================== Helper Functions ====================
void set_auth_headers(esp_http_client_handle_t client)
{
    if (strlen(API_TOKEN) > 0) {
        char auth_header[600];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", API_TOKEN);
        esp_http_client_set_header(client, "Authorization", auth_header);
        ESP_LOGI(API_TAG, "Using predefined API token");
        
    } else if (strlen(auth_token) > 0) {
        char auth_header[600];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", auth_token);
        esp_http_client_set_header(client, "Authorization", auth_header);
        ESP_LOGI(API_TAG, "Using token from login");
    } else {
        ESP_LOGW(API_TAG, "No token available!");
    }
}

// ==================== API Functions ====================

// Login
esp_err_t api_login(void)
{
    ESP_LOGI(API_TAG, "Logging in with email: %s", API_EMAIL);
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/login", API_BASE_URL);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "email", API_EMAIL);
    cJSON_AddStringToObject(root, "password", API_PASSWORD);
    char *post_data = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization",API_TOKEN);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Login response. Status: %d, Response: %s", status, http_response_buffer);
        
        if (status == 200) {
            cJSON *json = cJSON_Parse(http_response_buffer);
            if (json != NULL) {
                cJSON *token = cJSON_GetObjectItem(json, "token");
                if (token && token->valuestring) {
                    strncpy(auth_token, token->valuestring, sizeof(auth_token) - 1);
                    ESP_LOGI(API_TAG, " Login successful! Token saved.");
                } else {
                    ESP_LOGW(API_TAG, "No token in response");
                }
                cJSON_Delete(json);
            }
        }
    } else {
        ESP_LOGE(API_TAG, "Failed to login: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
    
    return err;
}

// Register Device
esp_err_t api_register_device(void)
{
    ESP_LOGI(API_TAG, "Registering device...");
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/device/register", API_BASE_URL);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", DEVICE_NAME2);
    cJSON_AddStringToObject(root, "serial", DEVICE_SERIAL);
    cJSON_AddStringToObject(root, "mac", DEVICE_MAC);
    char *post_data = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    set_auth_headers(client);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Device registered. Status: %d, Response: %s", status, http_response_buffer);
    } else {
        ESP_LOGE(API_TAG, "Failed to register device: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
    
    return err;
}

// Send Heartbeat
esp_err_t api_send_heartbeat(void)
{
    ESP_LOGI(API_TAG, "Sending heartbeat...");
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/device/%s/heartbeat", API_BASE_URL, DEVICE_ID);
    
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddStringToObject(root, "status", "online");
    char *post_data = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    set_auth_headers(client);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Heartbeat sent. Status: %d", status);
    } else {
        ESP_LOGE(API_TAG, "Failed to send heartbeat: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
    
    return err;
}

// Assign Service
esp_err_t api_assign_service(int service_id)
{
    ESP_LOGI(API_TAG, "Assigning service %d...", service_id);
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/device/%s/assign-service", API_BASE_URL, DEVICE_ID);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "service_id", service_id);
    char *post_data = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    set_auth_headers(client);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Service assigned. Status: %d", status);
    } else {
        ESP_LOGE(API_TAG, "Failed to assign service: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
    
    return err;
}

// Get Config
esp_err_t api_get_config(void)
{
    ESP_LOGI(API_TAG, "Getting device config...");
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/device/%s/config", API_BASE_URL, DEVICE_ID);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    set_auth_headers(client);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Config received. Status: %d, Response: %s", status, http_response_buffer);
        
        cJSON *json = cJSON_Parse(http_response_buffer);
        if (json != NULL) {
            cJSON *op_mode = cJSON_GetObjectItem(json, "op_mode");
            if (op_mode) {
                ESP_LOGI(TAG, "Operation Mode: %s", op_mode->valuestring);
            }
            cJSON_Delete(json);
        }
    } else {
        ESP_LOGE(API_TAG, "Failed to get config: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

// Store Feedback
esp_err_t api_store_feedback(int service_id, int rating_value)
{
    ESP_LOGI(API_TAG, "Storing feedback: service=%d, value=%d", service_id, rating_value);
    
    memset(http_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
    
    char url[256];
    snprintf(url, sizeof(url), "%s/feedback/store", API_BASE_URL);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "device_id", atoi(DEVICE_ID));
    cJSON_AddNumberToObject(root, "service_id", service_id);
    cJSON_AddNumberToObject(root, "value", rating_value);
    char *post_data = cJSON_PrintUnformatted(root);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 1024,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    set_auth_headers(client);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(API_TAG, "Feedback stored. Status: %d", status);
        
    } else {
        ESP_LOGE(API_TAG, "Failed to store feedback: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
    
    return err;
}
void api_task(void *pvParameters)
{
    ESP_LOGI(TAG, "API task started");

    while (!start_api) {
        
        ESP_LOGI(TAG, "Waiting for WiFi...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    start_api=0;
    
   // xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    if (api_login() == ESP_OK) {
        
      //  api_register_device();
      //  api_assign_service(3);
      //  api_get_config();
      ESP_LOGE(TAG, "Login successfully");
    } else {
        ESP_LOGE(TAG, "Login failed, will retry later");
    }

    while (1) {
        /*
        api_send_heartbeat();

        if (rate == 1) {
            rate = 0;
            api_store_feedback(3, score);
        }
*/
        
    }
    vTaskDelay(pdMS_TO_TICKS(10000)); 
}


void app_main()
{
    // Initialize the Non-Volatile Storage (NVS) for settings and data persistence
    // This ensures that user data and settings are retained even after power loss.
  //  init_nvs();
  //////////////
 

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and re-initialize if no free pages or new version found
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

  // ble_init();
    
    // Open NVS for reading
    /*
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return false;
    }
*/
    // Read username from NVS
    /*
    size_t username_size = MAX_LENGTH;
    err = nvs_get_str(nvs_handle, USERNAME_KEY, saved_username, &username_size);
    if (err != ESP_OK) {
        printf("Read failed!\n");
        nvs_close(nvs_handle);
        return false;
    }

    nvs_close(nvs_handle);

    return err;

*/


  ////////////////

    //static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
    //static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

    // Initialize the GT911 touch screen controller
    // This sets up the touch functionality of the screen.
    tp_handle = touch_gt911_init();

    // Initialize the Waveshare ESP32-S3 RGB LCD hardware
    // This prepares the LCD panel for display operations.
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();

    // Turn on the LCD backlight
    // This ensures the display is visible.
    wavesahre_rgb_lcd_bl_on();

    // Initialize the LVGL library, linking it to the LCD and touch panel handles
    // LVGL is a lightweight graphics library used for rendering UI elements.
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));

    ESP_LOGI(TAG, "Display LVGL demos");

    

    // Lock the LVGL port to ensure thread safety during API calls
    // This prevents concurrent access issues when using LVGL functions.
    if (lvgl_port_lock(-1))
    {

        // Initialize the UI components with LVGL (e.g., demo screens, sliders)
        // This sets up the user interface elements using the LVGL library.
        ui_init();

        // Release the mutex after LVGL operations are complete
        // This allows other tasks to access the LVGL port.
        lvgl_port_unlock();
    }
    vTaskDelay(100); // Delay for a short period to ensure stable initialization

    // Initialize PWM for controlling backlight brightness (if applicable)
    // PWM is used to adjust the brightness of the LCD backlight.
    //pwm_init();

    // Initialize SD card operations
    // This sets up the Micro SD card for data storage and retrieval.
    //sd_init();

    // Start the WIFI task to handle Wi-Fi functionality
    // This task manages Wi-Fi connections and hotspot creation.
    xTaskCreate(wifi_task, "wifi_task", 6 * 1024, NULL, 9, &wifi_TaskHandle);
    

     xTaskCreate(ble_server_task, "ble_server_task", 8 * 1024, NULL, 10, NULL);
     xTaskCreate(mainscreen_wifi_rssi_task, "mainscreen_wifi_rssi_task", 8* 1024, NULL, 9, NULL);
    // xTaskCreate(&api_task, "api_task", 8192, NULL, 5, NULL);
     //xTaskCreate(send_message_task, "send_message_task", 8 * 1024, NULL, 10, NULL);
     


    






}