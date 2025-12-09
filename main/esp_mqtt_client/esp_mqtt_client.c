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


static int extract_counter_number(const char *name) {
    if (!name) return -1;

    const char *ptr=name;
    
   
    
    if (ptr) {
        while (*ptr == ' ' || *ptr == ':' || *ptr == '-'||*ptr < '0' || *ptr > '9') {
            ptr++;
        }
        
        if (*ptr >= '0' && *ptr <= '9') {
            int num = atoi(ptr);
            if (num >= 1 && num <= 50) {
                return num;
            }
        }
    }
    
    return -1; 
}



static void sort_device_list_by_counter(int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {

            int c1 = extract_counter_number(device_list[j].name);
            int c2 = extract_counter_number(device_list[j+1].name);
            char a[3];
            sprintf(a,"%d",c1);
            char b[3];
            sprintf(b,"%d",c2);

            strncpy(device_list[j].name,a,sizeof(device_list[j].name));
            strncpy(device_list[j+1].name,b,sizeof(device_list[j+1].name));


            //int c1 = atoi(device_list[j].name);
            //int c2 = atoi(device_list[j + 1].name);

            if (c1 > c2) {
                device_info_t temp = device_list[j];
                device_list[j] = device_list[j + 1];
                device_list[j + 1] = temp;
            }
        }
    }
}



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



void save_device_list_to_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("DEVICE_LIST", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("TAG_NVS", "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    sort_device_list_by_counter(device_count);

    cJSON *root = cJSON_CreateArray();
    for (int i = 0; i < device_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", device_list[i].name);
        cJSON_AddStringToObject(item, "device_id", device_list[i].device_id);
        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        err = nvs_set_str(nvs_handle, "device_list", json_str);
        if (err == ESP_OK) {
            nvs_commit(nvs_handle);
            ESP_LOGI("TAG_NVS", "Saved %d devices to NVS", device_count);
        } else {
            ESP_LOGE("TAG_NVS", "Failed to save JSON: %s", esp_err_to_name(err));
        }
        free(json_str);
    }

    cJSON_Delete(root);
    nvs_close(nvs_handle);
}

esp_err_t load_device_list_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("DEVICE_LIST", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW("TAG_NVS", "No saved device list found");
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_str(nvs_handle, "device_list", NULL, &required_size);
    if (err != ESP_OK || required_size == 0) {
        ESP_LOGW("TAG_NVS", "No data for device_list_json");
        nvs_close(nvs_handle);
        return err;
    }

    char *json_str = malloc(required_size);
    if (!json_str) {
        ESP_LOGE("TAG_NVS", "Failed to allocate memory");
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, "device_list", json_str, &required_size);
    nvs_close(nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE("TAG_NVS", "Failed to read NVS data: %s", esp_err_to_name(err));
        free(json_str);
        return err;
    }

    ESP_LOGI("TAG_NVS", "Loaded JSON from NVS: %s", json_str);

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root || !cJSON_IsArray(root)) {
        ESP_LOGE("TAG_NVS", "Invalid JSON structure");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    memset(device_list, 0, sizeof(device_list));
    int count = 0;
    cJSON *item = NULL;

    cJSON_ArrayForEach(item, root) {
        if (count >= MAX_DEVICES) break;
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *id = cJSON_GetObjectItem(item, "device_id");
        if (cJSON_IsString(name) && cJSON_IsString(id)) {
            strncpy(device_list[count].name, name->valuestring, MAX_NAME_LEN - 1);
            strncpy(device_list[count].device_id, id->valuestring, MAX_ID_LEN - 1);
            count++;
        }
    }

    device_count = count;
    //sort_device_list_by_counter(device_count);//
    cJSON_Delete(root);

    ESP_LOGI("TAG_NVS", "Loaded %d devices from NVS", device_count);
    return ESP_OK;
}

esp_err_t load_selected_device_id(char *id_buf, size_t buf_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("SAVE_DEVICE_ID", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("LOAD_DEVICE_ID", "Cannot open NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = buf_size;
    err = nvs_get_str(nvs_handle, "device_id", id_buf, &required_size);
    if (err == ESP_OK) {
        ESP_LOGI("LOAD_DEVICE_ID", "Loaded device_id: %s", id_buf);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW("LOAD_DEVICE_ID", "No device_id saved");
    } else {
        ESP_LOGE("LOAD_DEVICE_ID", "Get failed: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

void parse_json_and_store(const char *json_data)
{
    cJSON *root = cJSON_Parse(json_data);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return;
    }

    if (!cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "Root is not an array");
        cJSON_Delete(root);
        return;
    }

    cJSON *item = NULL;
    uint8_t count = 0;

    cJSON_ArrayForEach(item, root)
    {
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *id = cJSON_GetObjectItem(item, "id");
        if (name && id && count < MAX_DEVICES)
        {
            strncpy(device_list[count].name, name->valuestring, sizeof(device_list[count].name) - 1);
            strncpy(device_list[count].device_id, id->valuestring, sizeof(device_list[count].device_id) - 1);
            count++;
        }
    }

    device_count = count;   
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Stored %d devices", device_count);
}



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

