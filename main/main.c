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
#include "mqtt_client.h"
#include "esp_crt_bundle.h"

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
//#define API_TOKEN "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJodHRwOi8vMTI3LjAuMC4xOjEyMDAxL2FwaS9sb2dpbiIsImlhdCI6MTc1OTIwODEyNywiZXhwIjoxNzU5Mjk0NTI3LCJuYmYiOjE3NTkyMDgxMjcsImp0aSI6IjdhSmhXMWtZMUFYT05rYnIiLCJzdWIiOiIxIiwicHJ2IjoiMjNiZDVjODk0OWY2MDBhZGIzOWU3MDFjNDAwODcyZGI3YTU5NzZmNyJ9.ZDfUOCN-PdhGfdb1ThwVbFfHBn_CGLYWZRGqHjUHmLA"                     
#define API_TOKEN ""
#define MAX_HTTP_OUTPUT_BUFFER 2048
char http_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
char auth_token[512] = {0};


static const char *TAG = "main"; // Tag used for ESP log output
static const char *API_TAG = "API_CLIENT";
static const char *MQTT_TAG ="MQTT";

static esp_lcd_panel_handle_t panel_handle = NULL; // Handle for the LCD panel
static esp_lcd_touch_handle_t tp_handle = NULL;    // Handle for the touch panel

extern int rate;
extern int score;

extern int mesh_enb;

 extern EventGroupHandle_t wifi_event_group;
extern int WIFI_CONNECTED_BIT ;

 extern int start_api;

esp_mqtt_client_handle_t mqttClient;

int mqtt=0;


 //LV_USE_PERF_MONITOR 


//extern const uint8_t hivemq_root_ca_pem_start[] asm("_binary_hivemq_root_ca_pem_start");
//extern const uint8_t hivemq_root_ca_pem_end[]   asm("_binary_hivemq_root_ca_pem_end");

// Khai báo để linker nhúng dữ liệu PEM vào firmware
//extern const uint8_t trustid_x3_root_pem_start[] asm("_binary_trustid_x3_root_pem_start");
//extern const uint8_t trustid_x3_root_pem_end[]   asm("_binary_trustid_x3_root_pem_end");

//extern const uint8_t isrgrootx1_pem_start[] asm("_binary_isrgrootx1_pem_start");
//extern const uint8_t isrgrootx1_pem_end[]   asm("_binary_isrgrootx1_pem_end");


const char mqtt_ca_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
"MIIFBTCCAu2gAwIBAgIQWgDyEtjUtIDzkkFX6imDBTANBgkqhkiG9w0BAQsFADBP\n"
"MQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFy\n"
"Y2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMTAeFw0yNDAzMTMwMDAwMDBa\n"
"Fw0yNzAzMTIyMzU5NTlaMDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBF\n"
"bmNyeXB0MQwwCgYDVQQDEwNSMTMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"
"AoIBAQClZ3CN0FaBZBUXYc25BtStGZCMJlA3mBZjklTb2cyEBZPs0+wIG6BgUUNI\n"
"fSvHSJaetC3ancgnO1ehn6vw1g7UDjDKb5ux0daknTI+WE41b0VYaHEX/D7YXYKg\n"
"L7JRbLAaXbhZzjVlyIuhrxA3/+OcXcJJFzT/jCuLjfC8cSyTDB0FxLrHzarJXnzR\n"
"yQH3nAP2/Apd9Np75tt2QnDr9E0i2gB3b9bJXxf92nUupVcM9upctuBzpWjPoXTi\n"
"dYJ+EJ/B9aLrAek4sQpEzNPCifVJNYIKNLMc6YjCR06CDgo28EdPivEpBHXazeGa\n"
"XP9enZiVuppD0EqiFwUBBDDTMrOPAgMBAAGjgfgwgfUwDgYDVR0PAQH/BAQDAgGG\n"
"MB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATASBgNVHRMBAf8ECDAGAQH/\n"
"AgEAMB0GA1UdDgQWBBTnq58PLDOgU9NeT3jIsoQOO9aSMzAfBgNVHSMEGDAWgBR5\n"
"tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcBAQQmMCQwIgYIKwYBBQUHMAKG\n"
"Fmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0gBAwwCjAIBgZngQwBAgEwJwYD\n"
"VR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVuY3Iub3JnLzANBgkqhkiG9w0B\n"
"AQsFAAOCAgEAUTdYUqEimzW7TbrOypLqCfL7VOwYf/Q79OH5cHLCZeggfQhDconl\n"
"k7Kgh8b0vi+/XuWu7CN8n/UPeg1vo3G+taXirrytthQinAHGwc/UdbOygJa9zuBc\n"
"VyqoH3CXTXDInT+8a+c3aEVMJ2St+pSn4ed+WkDp8ijsijvEyFwE47hulW0Ltzjg\n"
"9fOV5Pmrg/zxWbRuL+k0DBDHEJennCsAen7c35Pmx7jpmJ/HtgRhcnz0yjSBvyIw\n"
"6L1QIupkCv2SBODT/xDD3gfQQyKv6roV4G2EhfEyAsWpmojxjCUCGiyg97FvDtm/\n"
"NK2LSc9lybKxB73I2+P2G3CaWpvvpAiHCVu30jW8GCxKdfhsXtnIy2imskQqVZ2m\n"
"0Pmxobb28Tucr7xBK7CtwvPrb79os7u2XP3O5f9b/H66GNyRrglRXlrYjI1oGYL/\n"
"f4I1n/Sgusda6WvA6C190kxjU15Y12mHU4+BxyR9cx2hhGS9fAjMZKJss28qxvz6\n"
"Axu4CaDmRNZpK/pQrXF17yXCXkmEWgvSOEZy6Z9pcbLIVEGckV/iVeq0AOo2pkg9\n"
"p4QRIy0tK2diRENLSF2KysFwbY6B26BFeFs3v1sYVRhFW9nLkOrQVporCS0KyZmf\n"
"wVD89qSTlnctLcZnIavjKsKUu1nA1iU0yYMdYepKR7lWbnwhdx3ewok=\n"
"-----END CERTIFICATE-----\n";







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


///////// mqtt


void mqtt_publish_task (void *pvParameters)
{
	char datatoSend[20];
	
        if (mesh_enb&&mqtt){
            mesh_enb=0;
		int val = esp_random()%100;
		//sprintf(datatoSend, "%d", val);
        sprintf(datatoSend, "%s", "con cho Duy");
		//int msg_id = esp_mqtt_client_publish(mqttClient, "controllerstech/test1", datatoSend, 0, 0, 0);
		int msg_id = esp_mqtt_client_publish(mqttClient, "demo/laravel-001", datatoSend, 0, 0, 0);

        if (msg_id == 0) ESP_LOGI(TAG, "Sent Data: %d", val);
		else ESP_LOGI(TAG, "Error msg_id:%d while sending data", msg_id);
        }
    while (1){
		vTaskDelay(pdMS_TO_TICKS(2000));
	}

}


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        //xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 7, NULL);
        mqtt=1;

        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        break;
        
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(MQTT_TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(MQTT_TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(MQTT_TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(MQTT_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(MQTT_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
        
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}



void mqtt_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            //.address.uri = "mqtt://broker.hivemq.com",// exam
            //.address.port = 1883 //exam
           // .address.uri = "mqtts://0b2c802533b54670a78953e3c5758528.s1.eu.hivemq.cloud",
            .address.port = 1883,
            .address.uri = "mqtt://10.10.1.45",
           // .address.port = 1883,
            //.verification.certificate = (const char *)trustid_x3_root_pem_start,
         //  .verification.certificate = (const char *)mqtt_ca_cert_pem,
          //  .verification.certificate_len = strlen(isrgrootx1_pem_start) + 1,
             //.verification.certificate_len = isrgrootx1_pem_end - isrgrootx1_pem_start,
                 //    .verification.certificate =      mqtt_cert_pem,
                      
              
        },

        .credentials = {
            .username = "appuser",
            .authentication.password = "111111",
        },
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}






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

    while (!connection_flag) {
        
        ESP_LOGI(TAG, "Waiting for WiFi...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
   
    
    //xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);


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
*/         vTaskDelay(pdMS_TO_TICKS(2000)); 
        
    }
    
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
    xTaskCreate(wifi_task, "wifi_task", 8 * 1024, NULL, 9, &wifi_TaskHandle);
    

     xTaskCreate(ble_server_task, "ble_server_task", 8 * 1024, NULL, 10, NULL);
     xTaskCreate(mainscreen_wifi_rssi_task, "mainscreen_wifi_rssi_task", 4* 1024, NULL, 9, NULL);
 //    xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 7, NULL);
     //xTaskCreate(&api_task, "api_task", 1024*4, NULL, 5, NULL);
     //xTaskCreate(send_message_task, "send_message_task", 8 * 1024, NULL, 10, NULL);
    // if (connection_flag){
          
    // }
     


    






}