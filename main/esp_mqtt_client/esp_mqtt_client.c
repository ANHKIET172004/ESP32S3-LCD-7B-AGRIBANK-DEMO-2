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

uint8_t key_id=0;

bool stop=false;

char selected_keypad_id[19]={0};

device_info_t device_list[MAX_DEVICES];
int device_count = 0;

extern lv_obj_t * ui_Label1;
extern lv_obj_t * ui_Image2;
extern QueueHandle_t mqtt_queue;

extern esp_err_t delete_current_number_from_nvs(void) ;


static void handle_response_number(const char *payload)
{
    if (stop) return;

    cJSON *root = cJSON_Parse(payload);
    if (!root) {
        ESP_LOGE(MQTT_TAG, "Parse JSON failed (responsenumber)");
        return;
    }

    cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
    cJSON *number_item    = cJSON_GetObjectItem(root, "number");

    if (!cJSON_IsString(device_id_item) || !cJSON_IsString(number_item)) {
        ESP_LOGW(MQTT_TAG, "Invalid JSON fields in responsenumber");
        cJSON_Delete(root);
        return;
    }

    if (load_selected_device_id(selected_keypad_id, sizeof(selected_keypad_id)) != ESP_OK) {
        selected_keypad_id[0] = '\0';
    }

    const char *number_str = number_item->valuestring;

    if ((strcmp(device_id_item->valuestring, selected_keypad_id) == 0) &&
        (strcmp(number_str, "NoAvailable") != 0)) {

        if (lvgl_port_lock(-1)) {
            lv_label_set_text(ui_Label1, number_str);
            lv_obj_set_style_text_font(ui_Label1,&ui_font_BOLDVN250,LV_PART_MAIN | LV_STATE_DEFAULT
            );
            lvgl_port_unlock();
        }

        ESP_LOGI(MQTT_TAG, "Updated number: %s", number_str);
        save_called_number(number_str);
        save_status("NO");
    }

    cJSON_Delete(root);
}

static void handle_reset_number(void)
{
    stop = false;

    if (lvgl_port_lock(-1)) {
        //lv_label_set_text(ui_Label1, "0000");
        lv_label_set_text(ui_Label1, "");
        lv_obj_set_style_text_font( ui_Label1,&ui_font_BOLDVN250,LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lvgl_port_unlock();
    }

    delete_current_number_from_nvs();
    save_status("NO");

    ESP_LOGI(MQTT_TAG, "Reset number");
}


static void handle_device_list(const char *payload)
{
    device_info_t new_list[MAX_DEVICES] = {0};
    int new_count = 0;

    if (parse_json_to_device_list(payload, new_list, &new_count) != ESP_OK) {
        ESP_LOGE(MQTT_TAG, "Parse device list failed");
        return;
    }

    build_new_list(new_list, new_count);

    device_info_t old_list[MAX_DEVICES] = {0};
    int old_count = 0;

    esp_err_t err = load_device_list_from_nvs_to_buffer(old_list, &old_count);

    qsort(new_list, new_count, sizeof(device_info_t), device_compare_by_counter);
    qsort(old_list, old_count, sizeof(device_info_t), device_compare_by_counter);

    bool need_save = false;

    if (err != ESP_OK) {
        need_save = true;
        ESP_LOGI(MQTT_TAG, "No old device list, save new");
    } 
    else if (device_list_is_different(new_list, new_count, old_list, old_count)) {
        need_save = true;
        ESP_LOGI(MQTT_TAG, "Device list changed");
    }

    if (need_save) {
        memcpy(device_list, new_list, sizeof(device_info_t) * new_count);
        device_count = new_count;
        save_device_list_to_nvs_from_buffer(new_list, new_count);
    } else {
        ESP_LOGI(MQTT_TAG, "Device list unchanged");
    }
}


static void handle_closed(void)
{
    stop = true;

    if (lvgl_port_lock(-1)) {
        lv_label_set_text(ui_Label1, "QUẦY TẠM THỜI ĐÓNG");
        lv_obj_set_style_text_font( ui_Label1, &ui_font_BOLDVN80, LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lvgl_port_unlock();
    }

    save_status("YES");
    ESP_LOGI(MQTT_TAG, "Device closed");
}

static void handle_open(void)
{
    stop = false;

    if (lvgl_port_lock(-1)) {
        //lv_label_set_text(ui_Label1, "0000");
        lv_label_set_text(ui_Label1, "");
        lv_obj_set_style_text_font( ui_Label1,&ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lvgl_port_unlock();
    }

    save_status("NO");
    ESP_LOGI(MQTT_TAG, "Device opened");
}

void mqtt_process_task(void *pvParameters)
{
    mqtt_message_t msg;

    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY)) {

            ESP_LOGI(MQTT_TAG, "Topic: %s", msg.topic);
            ESP_LOGI(MQTT_TAG, "Payload: %s", msg.data);

            if (strcmp(msg.topic, "responsenumber") == 0) {
                handle_response_number(msg.data);
            }
            else if (strcmp(msg.topic, "reset_number") == 0) {
                handle_reset_number();
            }
            else if (strcmp(msg.topic, "device/list") == 0) {
                handle_device_list(msg.data);
            }
            else if (strcmp(msg.topic, "closed") == 0) {
                handle_closed();
            }
            else if (strcmp(msg.topic, "open") == 0) {
                handle_open();
            }
            else {
                ESP_LOGW(MQTT_TAG, "Unhandled topic: %s", msg.topic);
            }
        }
    }
}



void mqtt_process_task1(void *pvParameters)
{
    mqtt_message_t msg;

    while (1) {
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY)) {

            ESP_LOGI(MQTT_TAG, "Processing topic: %s", msg.topic);
            ESP_LOGI(MQTT_TAG, "Data: %s", msg.data);

            cJSON *root=NULL;

            
            if ((strcmp(msg.topic, "responsenumber") == 0)) {
                root=cJSON_Parse(msg.data);// chỉ 2 topic có payload dạng json nên chỉ parse khi gặp 2 topic này
                if (!root){
                  ESP_LOGE(MQTT_TAG, "Failed to parse JSON");
                  continue;
                }
                
                
            }


            if ((strcmp(msg.topic, "responsenumber") == 0)&&(stop==false)) {
                cJSON *device_id_item = cJSON_GetObjectItem(root, "device_id");
                cJSON *number_item    = cJSON_GetObjectItem(root, "number");
                //cJSON *skip_item      = cJSON_GetObjectItem(root, "skip");
                if (!device_id_item || !number_item) {
                    ESP_LOGW(MQTT_TAG, "Missing fields in responsenumber JSON");
                    cJSON_Delete(root);
                    continue;
                }

                if (!cJSON_IsString(device_id_item) || !cJSON_IsString(number_item)) {
                    ESP_LOGW(MQTT_TAG, "Invalid field type in responsenumber JSON");
                    cJSON_Delete(root);
                    continue;
                }
               

                if (cJSON_IsString(device_id_item) && cJSON_IsString(number_item)) {

                    if (load_selected_device_id(selected_keypad_id, sizeof(selected_keypad_id)) != ESP_OK) {
                        selected_keypad_id[0] = '\0'; 
                    }
                        
                    const char *number_str = number_item->valuestring;

                    if ((strcmp(device_id_item->valuestring, selected_keypad_id) == 0)&&(strcmp(number_str, "NoAvailable") != 0)) {

                        if (lvgl_port_lock(-1)) {
                            lv_label_set_text(ui_Label1, number_str);
                            lv_obj_set_style_text_font(ui_Label1, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
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
                    //lv_label_set_text(ui_Label1, "0000");
                    lv_label_set_text(ui_Label1, "");
                    lv_obj_set_style_text_font(ui_Label1, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }
                delete_current_number_from_nvs();
                save_status("NO");


            }

             else if (strcmp(msg.topic, "device/list") == 0) {
                ESP_LOGI(TAG, "Device list received");
              //  parse_json_and_store(msg.data);   
              //  save_device_list_to_nvs();  
                
                device_info_t new_list[MAX_DEVICES] = {0};
                int new_count = 0;

                if (parse_json_to_device_list(msg.data, new_list, &new_count) != ESP_OK) {
                    ESP_LOGE(TAG, "Parse device list failed");
                    //return;
                    cJSON_Delete(root);
                    continue;
                }
                build_new_list(new_list,new_count);//

                device_info_t old_list[MAX_DEVICES] = {0};
                int old_count = 0;

                esp_err_t err = load_device_list_from_nvs_to_buffer(old_list, &old_count);
                
                qsort(new_list, new_count, sizeof(device_info_t), device_compare_by_counter);
                qsort(old_list, old_count, sizeof(device_info_t), device_compare_by_counter);


                bool need_save = false;

                if (err != ESP_OK) {
                    ESP_LOGI(TAG, "No device list in NVS, save new list");
                    need_save = true;
                } else if (device_list_is_different(new_list, new_count, old_list, old_count)) {
                    ESP_LOGI(TAG, "Device list changed, save new list");
                    need_save = true;
                } else {
                    ESP_LOGI(TAG, "Device list unchanged, skip save");
                }

                if (need_save) {
                    memcpy(device_list, new_list, sizeof(device_info_t) * new_count);
                    device_count = new_count;
                    //save_device_list_to_nvs();
                    save_device_list_to_nvs_from_buffer(new_list,new_count);//
                } 
                     
            }

            else if (strcmp(msg.topic,"closed")==0){
                if (lvgl_port_lock(-1)) {
                    stop=true;
                    lv_label_set_text(ui_Label1, "QUẦY TẠM THỜI ĐÓNG");
                    //save_status("YES");
                    lv_obj_set_style_text_font(ui_Label1, &ui_font_BOLDVN80, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }
            }

            else if (strcmp(msg.topic,"open")==0){
               if (lvgl_port_lock(-1)) {
                    stop=false;
                    //lv_label_set_text(ui_Label1, "0000");
                    lv_label_set_text(ui_Label1, "");
                    save_status("NO");
                    lv_obj_set_style_text_font(ui_Label1, &ui_font_BOLDVN250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lvgl_port_unlock();
                }

            }


            else {
                ESP_LOGI(MQTT_TAG, "Unhandled topic: %s", msg.topic);
            }
        if (root){
            cJSON_Delete(root);
         }
        }
    }
}



void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");

        esp_mqtt_client_subscribe(event->client, "responsenumber", 0);
        esp_mqtt_client_subscribe(event->client, "reset_number", 0);// reset số về 0 khi nhận mess từ bàn phím
        esp_mqtt_client_subscribe(event->client, "closed", 0);// đóng qầy
        esp_mqtt_client_subscribe(event->client, "open", 0);// mở qầy
        esp_mqtt_client_subscribe(event->client, "device/list", 1);// nhận device list
        if (lvgl_port_lock(-1)){
        lv_obj_add_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN ); 
        lvgl_port_unlock();
        }


        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        if (lvgl_port_lock(-1)){
        lv_obj_clear_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN ); 
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
        lv_obj_clear_flag(ui_Image2, LV_OBJ_FLAG_HIDDEN ); //
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



void mqtt_start(void)// khởi tạo mqtt
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

        .network.disable_auto_reconnect = false,
        
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    mqttClient = client;
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

