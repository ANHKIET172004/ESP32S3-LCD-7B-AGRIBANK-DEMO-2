/*****************************************************************************
 * | File       :   main.c
 * | Author     :   7 Button MQTT Display
 * | Function   :   Hiển thị số + 7 nút điều khiển
 * | Info       :   ESP32-S3 + 7" LCD + MQTT + LVGL
 * | Version    :   V3.2
 * | Date       :   2025-01-10
 ******************************************************************************/

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
#include "cJSON.h"

#include "rgb_lcd_port.h"
#include "gt911.h"
#include "lvgl_port.h"

#include "ui.h"

#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI/main/gatt_client/gatt_client.h"
//#include "lv_conf.h"
//#include "D:/ESP32/ESP32S3_LCD 7B_DEMO_3/ESP32-S3-Touch-LCD-7B-Demo/ESP32-S3-Touch-LCD-7B-Demo/ESP-IDF/16_LVGL_UI 2/main/lv_conf.h"
#include "wifi.h"
#include "esp_mqtt_client/esp_mqtt_client.h"
// ==================== CẤU HÌNH ====================

#define WIFI_SSID           "NamTrung"
#define WIFI_PASSWORD       "nt@14092011"
#define WIFI_MAXIMUM_RETRY  5

#define MQTT_BROKER_URI     "mqtt://10.10.1.27:1883"
#define MQTT_USERNAME       "appuser"
#define MQTT_PASSWORD       "1111"
#define MQTT_TOPIC_RESPONSE "responsenumber"
#define MQTT_TOPIC_REQUEST  "requestnumber"
#define DEVICE_ID           "04:1A:2B:3C:4D:04"

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

// ==================== BIẾN TOÀN CỤC ====================

static const char *TAG = "MAIN";
static const char *MQTT_TAG = "MQTT";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t tp_handle = NULL;

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static QueueHandle_t display_queue = NULL;
static esp_mqtt_client_handle_t mqtt_client = NULL;

// LVGL Objects
lv_obj_t *label_number = NULL;
lv_obj_t *buttons[7];

/////////
extern int mesh_enb;


//lv_obj_t * ui_Screen7 = NULL;
//lv_obj_t * ui_Panel15 = NULL;
//lv_obj_t * ui_Label25 = NULL;
//lv_obj_t * ui_Label24 = NULL;



// ==================== QUEUE MESSAGE ====================

typedef struct {
    int number;
} display_message_t;

display_message_t msg;

// ==================== BUTTON CALLBACKS ====================

static void btn_event_cb_1(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        ESP_LOGI(MQTT_TAG, "Nút 1: Yêu cầu số ngẫu nhiên");
        const char *json = "{\"device_id\":\"00:1A:2B:3C:4D:09\",\"name\":\"Device-01\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}

static void btn_event_cb_2(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        const char *json = "{\"device_id\":\"10:20:BA:01:48:A8\",\"name\":\"Device-02\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);

    }
}

static void btn_event_cb_3(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        const char *json = "{\"device_id\":\"02:1A:2B:3C:4D:02\",\"name\":\"Device-03\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}

static void btn_event_cb_4(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        const char *json = "{\"device_id\":\"04:1A:2B:3C:4D:04\",\"name\":\"Device-04\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}

static void btn_event_cb_5(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        const char *json = "{\"device_id\":\"00:1A:2B:3C:4D:05\",\"name\":\"Device-05\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}

static void btn_event_cb_6(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        ESP_LOGI(MQTT_TAG, "Nút 6: Nhân x2");
        const char *json = "{\"device_id\":\"00:1A:2B:3C:4D:06\",\"name\":\"Device-06\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}

static void btn_event_cb_7(lv_event_t *e) {
    if (e->code == LV_EVENT_CLICKED) {
        ESP_LOGI(MQTT_TAG, "Nút 6: Nhân x2");
        const char *json = "{\"device_id\":\"00:1A:2B:3C:4D:07\",\"name\":\"Device-07\"}";
        esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_REQUEST, json, 0, 0, 0);
    }
}





/*
void ui_Screen7_screen_init(void)
{
    //ui_Screen7 = lv_obj_create(NULL);
    
    ui_Screen7 = lv_scr_act();
    lv_obj_clear_flag(ui_Screen7, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Panel15 = lv_obj_create(ui_Screen7);
    lv_obj_set_width(ui_Panel15, 1017);
    lv_obj_set_height(ui_Panel15, 146);
    lv_obj_set_x(ui_Panel15, -1);
    lv_obj_set_y(ui_Panel15, -226);
    lv_obj_set_align(ui_Panel15, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel15, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_Panel15, lv_color_hex(0x189FD7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Panel15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label25 = lv_label_create(ui_Panel15);
    lv_obj_set_width(ui_Label25, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label25, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label25, -12);
    lv_obj_set_y(ui_Label25, -8);
    lv_obj_set_align(ui_Label25, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label25, "QUẦY 1");
    lv_obj_set_style_text_font(ui_Label25, &ui_font_BOLDVN60, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label24 = lv_label_create(ui_Screen7);
    lv_obj_set_width(ui_Label24, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label24, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label24, 20);
    lv_obj_set_y(ui_Label24, 74);
    lv_obj_set_align(ui_Label24, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label24, "10002 ");
    lv_obj_set_style_text_color(ui_Label24, lv_color_hex(0xCC2E2E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label24, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label24, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);



    lv_obj_t * area = lv_obj_create(ui_Screen7);
    lv_obj_set_size(area, 100, 100);
    //lv_obj_set_style_bg_color(area, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(area, LV_OPA_TRANSP, LV_PART_MAIN);   // Ẩn nền
    lv_obj_set_style_border_opa(area, LV_OPA_TRANSP, LV_PART_MAIN); // Ẩn viền
    lv_obj_set_style_outline_opa(area, LV_OPA_TRANSP, LV_PART_MAIN); // Ẩn viền ngoài
    lv_obj_align(area, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_add_event_cb(area,area_click_event_cb,LV_EVENT_CLICKED,NULL);



}
*/
// ==================== LVGL UI ====================

void create_ui_with_7_buttons(void) {
    lv_obj_t *screen = lv_scr_act();
    
    // Background đen
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), 0);
    
    // Panel hiển thị số - ở trên
    lv_obj_t *number_panel = lv_obj_create(screen);
    lv_obj_set_size(number_panel, 700, 200);
    lv_obj_set_pos(number_panel, 162, 30);
    lv_obj_set_style_bg_color(number_panel, lv_color_hex(0Xffffff), 0);
    //lv_obj_set_style_border_color(number_panel, lv_color_hex(0x00D9FF), 0);
    lv_obj_set_style_border_width(number_panel, 3, 0);
    lv_obj_set_style_radius(number_panel, 20, 0);
    lv_obj_clear_flag(number_panel, LV_OBJ_FLAG_SCROLLABLE);
    
    // Label số
    label_number = lv_label_create(number_panel);
    lv_label_set_text(label_number, "---");
    //lv_obj_set_style_text_font(label_number, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_font(label_number, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_number, lv_color_hex(0x0000000), 0);
    lv_obj_center(label_number);
    
    // Cấu hình 7 nút
    const char *btn_labels[] = {
        "1",
        "2", 
        "3",
        "4",
        "5",
        "6",
        "7"
    };
    
    uint32_t btn_colors[] = {
        0x2196F3, 
        0x2196F3, 
        0x2196F3, 
        0x2196F3, 
        0x2196F3, 
        0x2196F3, 
        0x2196F3  
    };
    
    void (*btn_callbacks[])(lv_event_t *) = {
        btn_event_cb_1,
        btn_event_cb_2,
        btn_event_cb_3,
        btn_event_cb_4,
        btn_event_cb_5,
        btn_event_cb_6,
        btn_event_cb_7
    };
    
    // Tạo 7 nút xếp thành 2 hàng
    int btn_width = 140;
    int btn_height = 80;
    int spacing = 10;
    int start_x = 32;
    int row1_y = 260;
    int row2_y = 360;
    
    for (int i = 0; i < 7; i++) {
        buttons[i] = lv_btn_create(screen);
        lv_obj_set_size(buttons[i], btn_width, btn_height);
        
        // Hàng 1: 4 nút, Hàng 2: 3 nút (căn giữa)
        if (i < 4) {
            lv_obj_set_pos(buttons[i], start_x + i * (btn_width + spacing), row1_y);
        } else {
            int offset = (btn_width + spacing) / 2; // Để căn giữa 3 nút
            lv_obj_set_pos(buttons[i], start_x + offset + (i - 4) * (btn_width + spacing), row2_y);
        }
        
        lv_obj_set_style_bg_color(buttons[i], lv_color_hex(btn_colors[i]), 0);
        lv_obj_set_style_radius(buttons[i], 15, 0);
        
        // Label trên nút
        lv_obj_t *label = lv_label_create(buttons[i]);
        lv_label_set_text(label, btn_labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_center(label);
        
        // Gắn callback
        lv_obj_add_event_cb(buttons[i], btn_callbacks[i], LV_EVENT_CLICKED, NULL);
    }
    
    ESP_LOGI(TAG, "UI với 7 nút đã tạo");
}

// ==================== LVGL UPDATE TASK ====================

void lvgl_update_task(void *pvParameters) {
    display_message_t msg;
    char buffer[32];
    
    ESP_LOGI(TAG, "Started LVGL update task");
    
    while (1) {
        if (xQueueReceive(display_queue, &msg, portMAX_DELAY) == pdTRUE) {
            
            if (lvgl_port_lock(100)) {
                snprintf(buffer, sizeof(buffer), "%d", msg.number);
                //lv_label_set_text(label_number, buffer);
                lv_label_set_text(ui_Label26, buffer);//
                
                lvgl_port_unlock();
                
                ESP_LOGI(TAG, "Display number: %d", msg.number);
            }
        }
    }
}

// ==================== WIFI EVENT HANDLER ====================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi bắt đầu kết nối");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Thử kết nối lại WiFi %d/%d", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Kết nối WiFi thất bại");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Đã có IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ==================== WIFI INIT ====================

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init xong");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Kết nối WiFi thành công");
    } else {
        ESP_LOGE(TAG, "Kết nối WiFi thất bại");
    }
}

// ==================== MQTT EVENT HANDLER ====================

// ==================== APP MAIN ====================

void app_main(void) {
    //ESP_LOGI(TAG, "=== ESP32 MQTT 7 BUTTONS ===");

    // 1. NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "✓ NVS OK");

    // 2. Touch
    tp_handle = touch_gt911_init();
    ESP_LOGI(TAG, "✓ Touch OK");

    // 3. LCD
    panel_handle = waveshare_esp32_s3_rgb_lcd_init();
    wavesahre_rgb_lcd_bl_on();
    ESP_LOGI(TAG, "✓ LCD OK");

    // 4. LVGL
    ESP_ERROR_CHECK(lvgl_port_init(panel_handle, tp_handle));
    ESP_LOGI(TAG, "✓ LVGL OK");

    // 5. Queue
    display_queue = xQueueCreate(10, sizeof(display_message_t));
    ESP_LOGI(TAG, "✓ Queue OK");

    // BLE
  //  ble_init();
  //  ESP_LOGI(TAG, "✓ BLE OK");


    // 6. UI
    if (lvgl_port_lock(-1)) {
        //create_ui_with_7_buttons();
        //ui_Screen7_screen_init();
        ui_init();
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "create UI successfully");

    // 7. Task
    xTaskCreate(lvgl_update_task, "lvgl_update", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Task OK");

    // 8. WiFi
    //wifi_init_sta();
    xTaskCreatePinnedToCore(wifi_task, "wifi_task", 7* 1024, NULL, 6, &wifi_TaskHandle, 0);


    // 9. MQTT
    

    mqtt_start();
    
    
//    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
//    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, 
  //                                 mqtt_event_handler, NULL);
    //esp_mqtt_client_start(mqtt_client);
    //ESP_LOGI(TAG, "✓ MQTT OK");

    //ESP_LOGI(TAG, "=== SYSTEM READY - 7 BUTTONS ===");
    //xTaskCreate(send_message_task, "send_message_task", 8 * 1024, NULL, 10, NULL);
    
}