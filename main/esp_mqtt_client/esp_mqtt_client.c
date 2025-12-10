#include "esp_mqtt_client.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "ui.h"
#include "mqtt_data.h"

#define MAX_DEVICES 20
#define MAX_NAME_LEN 32
#define MAX_ID_LEN   32

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

static const char *TAG = "MQTT_BACKUP"; 
static const char *MQTT_TAG ="MQTT";

typedef struct {
    char topic[64];
    char data[512];
} mqtt_message_t;

esp_mqtt_client_handle_t mqttClient;

int mqtt=0;

uint8_t key_id=0;

bool stop=false;


char selected_device_id[19]={0};

char selected_keypad_id[19]={0};

device_info_t device_list[MAX_DEVICES];
int device_count = 0;


extern lv_obj_t * ui_Image20 ;
extern lv_obj_t * ui_Image24 ;
extern lv_obj_t * ui_Image31 ;
extern lv_obj_t * ui_Image34 ;
extern lv_obj_t * ui_Image32 ;
extern lv_obj_t * ui_Label26;
extern lv_obj_t * ui_Image70;
extern lv_obj_t * ui_Image38;
extern QueueHandle_t mqtt_queue;

extern esp_err_t delete_current_number_from_nvs(void) ;



void mqtt_process_task(void *pvParameters)
{
    mqtt_message_t msg;

    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY)) {

            ESP_LOGI(MQTT_TAG, "Processing topic: %s", msg.topic);
            ESP_LOGI(MQTT_TAG, "Data: %s", msg.data);

            cJSON *root = cJSON_Parse(msg.data);
            if ((!root)&&(strcmp(msg.topic, "responsenumber") == 0) ) {
                ESP_LOGE(MQTT_TAG, "Failed to parse JSON");
                continue;
            }

            if ((strcmp(msg.topic, "responsenumber") == 0)&&(stop==false)) {
                cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
                cJSON *number_item    = cJSON_GetObjectItem(root, "number");
                //cJSON *skip_item      = cJSON_GetObjectItem(root, "skip");

                if (cJSON_IsString(device_id_item) && cJSON_IsString(number_item)) {

                    if (load_selected_device_id(selected_keypad_id, sizeof(selected_keypad_id)) != ESP_OK) {
                        selected_keypad_id[0] = '\0'; 
                    }
                        
                    const char *number_str = number_item->valuestring;

                    if ((strcmp(device_id_item->valuestring, selected_keypad_id) == 0)&&(strcmp(number_str, "NoAvailable") != 0)) {
                        
                        


                        if (lvgl_port_lock(-1)) {
                        lv_label_set_text(ui_Label26, number_str);
                        lv_obj_set_style_text_font(ui_Label26, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
                        lvgl_port_unlock();
                    }   
                        ESP_LOGI(MQTT_TAG, "Updated label with number: %s", number_str);
                        save_called_number(number_str);
                        save_status("NO");



                    }
                } else {
                    ESP_LOGW(MQTT_TAG, "Invalid JSON fields in 'number' topic");
                }
            }

            else if (strcmp(msg.topic,"reset_number")==0){
                if (lvgl_port_lock(-1)) {
                    stop=false;//
                    lv_label_set_text(ui_Label26, "0000");
                    lv_obj_set_style_text_font(ui_Label26, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }
                delete_current_number_from_nvs();
                save_status("NO");


            }

             else if (strcmp(msg.topic, "device/list") == 0) {
                ESP_LOGI(TAG, "Device list received");
                parse_json_and_store(msg.data);   
                save_device_list_to_nvs();       
            }

            else if (strcmp(msg.topic,"closed")==0){
                if (lvgl_port_lock(-1)) {
                    stop=true;
                    lv_label_set_text(ui_Label26, "QUẦY TẠM THỜI ĐÓNG");
                    save_status("YES");
                    lv_obj_set_style_text_font(ui_Label26, &ui_font_BOLDVN80, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }

            }

            else if (strcmp(msg.topic,"open")==0){
               if (lvgl_port_lock(-1)) {
                    stop=false;
                    lv_label_set_text(ui_Label26, "0000");
                    save_status("NO");
                    lv_obj_set_style_text_font(ui_Label26, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }

            }


            else {
                ESP_LOGI(MQTT_TAG, "Unhandled topic: %s", msg.topic);
            }

            cJSON_Delete(root);
        }
    }
}



void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
                //////
                /*
        int msg_id = esp_mqtt_client_publish(mqttClient, "display_status", "connected", 0, 1, 1);
        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Sent connection message to keypad successfully, msg_id=%d",msg_id);
        } else {
            ESP_LOGW(TAG, "Send connection message to keypad failed!");
            //vTaskDelay(pdMS_TO_TICKS(1000));
        }
            */
        /////
        esp_mqtt_client_subscribe(event->client, "responsenumber", 0);
        esp_mqtt_client_subscribe(event->client, "reset_number", 0);
        esp_mqtt_client_subscribe(event->client, "closed", 0);
        esp_mqtt_client_subscribe(event->client, "open", 0);
        esp_mqtt_client_subscribe(event->client, "device/list", 1);
        if (lvgl_port_lock(-1)){
        lv_obj_add_flag(ui_Image70, LV_OBJ_FLAG_HIDDEN ); 
        lvgl_port_unlock();
        }


        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        if (lvgl_port_lock(-1)){
        lv_obj_clear_flag(ui_Image70, LV_OBJ_FLAG_HIDDEN ); 
        lvgl_port_unlock();
        }

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
         mqtt_message_t msg;
        int topic_len = MIN(event->topic_len, sizeof(msg.topic) - 1);
        int data_len  = MIN(event->data_len, sizeof(msg.data) - 1);

        memcpy(msg.topic, event->topic, topic_len);
        msg.topic[topic_len] = '\0';

        memcpy(msg.data, event->data, data_len);
        msg.data[data_len] = '\0';

        if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE) {
            ESP_LOGW(TAG, "MQTT queue full, message dropped");
        }
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        lv_obj_clear_flag(ui_Image70, LV_OBJ_FLAG_HIDDEN ); //
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

            .address.port = 1883,
            .address.uri = "mqtt://10.10.1.21",

        },

        .credentials = {
            .username = "appuser",
            .authentication.password = "1111",
        },
        /*
        .session = {
            //.keepalive = 60,
            .keepalive = 15,
            .disable_clean_session = false,
            .last_will.topic = "display_status",
            //.last_will.msg = "{\"device_id\":\"04:1A:2B:3C:4D:04\",\"status\":\"offline\"}",
            //.last_will.msg = "{\"device_id\":\"A8:42:E3:4C:7C:BC\",\"status\":\"offline\"}",
            .last_will.msg="disconnected",
            .last_will.qos = 1,
            .last_will.retain = true,
        },
        */
        .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

