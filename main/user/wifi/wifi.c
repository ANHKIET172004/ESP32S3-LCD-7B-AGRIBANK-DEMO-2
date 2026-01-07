#include "ui.h"
#include "rgb_lcd_port.h"
#include "wifi.h"
#include "lvgl_port.h"

#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mqtt_client.h"


const char *TAG_AP = "WiFi SoftAP";  // Tag for SoftAP mode
const char *TAG_STA = "WiFi Sta";    // Tag for Station mode

TaskHandle_t wifi_TaskHandle;
static wifi_sta_list_t sta_list;  // List to hold connected stations information


////////// các biến liên quan đến cấu hình wifi

extern wifi_config_t wifi_config;
extern char saved_ssid[32];
extern char saved_password[64];//
//////////
extern bool found_saved_ap;
extern int change;
extern int cnt;

uint8_t no_wifi=1;

extern esp_netif_t *sta_netif;


 char saved_ssid1[32]={0} ;
 char saved_pass1[32]={0} ;
 size_t ssid_len1=sizeof(saved_pass1);
 size_t password_len1=sizeof(saved_pass1) ;


esp_err_t read_wifi_credentials_from_nvs1(char *ssid, size_t *ssid_len, char *password, size_t *password_len,uint8_t* bssid) {
    nvs_handle_t my_handle;
    esp_err_t err;
    err = nvs_open("wifi_cred", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_str(my_handle, "ssid", ssid, ssid_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    err = nvs_get_str(my_handle, "password", password, password_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    size_t bssid_len = 6;
    err = nvs_get_blob(my_handle, "bssid", bssid, &bssid_len);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }
    nvs_close(my_handle);
    return ESP_OK;
}


// Callback function to update UI when Wi-Fi connection is established
/*
static void wifi_connection_cb(lv_timer_t *timer)
{
    // Hide the "waiting for connection" spinner and enable Wi-Fi related buttons
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
}
*/
 void wifi_connection_cb(lv_timer_t *timer)
{
    // Hide the "waiting for connection" spinner and enable Wi-Fi related buttons
    _ui_flag_modify(ui_WIFI_Wait_CONNECTION, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    _ui_state_modify(ui_WIFI_OPEN, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
}

// Callback function to update UI with connected stations information in SoftAP mode
static void wifi_ap_cb(lv_timer_t *timer)
{
    char mac_str[32];  // Buffer to hold formatted MAC address string
    snprintf(mac_str, sizeof(mac_str), "Connected: %d", sta_list.num); // Format the number of connected stations
    lv_label_set_text(ui_WIFI_AP_CON_NUM, mac_str); // Update the UI with the number of connected stations
    ESP_LOGI(TAG_AP, "Total connected stations: %d", sta_list.num);

    // Hide or show the MAC address list based on the number of connected stations
    if (sta_list.num == 0)
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_ADD);
    else
        _ui_flag_modify(ui_WIFI_AP_MAC_List, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);

    lv_obj_clean(ui_WIFI_AP_MAC_List);  // Clean the MAC address list to prevent duplicates

    // Iterate over the connected stations and display their MAC addresses
    for (int i = 0; i < sta_list.num; i++)
    {
        wifi_sta_info_t sta_info = sta_list.sta[i];

        // Format the MAC address and display it in the list
        snprintf(mac_str, sizeof(mac_str), "MAC: " MACSTR, MAC2STR(sta_info.mac));
        lv_obj_t * WIFI_AP_MAC_List_Button = lv_list_add_btn(ui_WIFI_AP_MAC_List, NULL, mac_str);

        // Customize button style
        lv_obj_set_style_bg_opa(WIFI_AP_MAC_List_Button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(WIFI_AP_MAC_List_Button, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(WIFI_AP_MAC_List_Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        //lv_obj_set_style_text_font(WIFI_AP_MAC_List_Button, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(WIFI_AP_MAC_List_Button, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Log information about the connected stations
        ESP_LOGI(TAG_AP, "STA %d: MAC Address: " MACSTR, i, MAC2STR(sta_info.mac));
        ESP_LOGI(TAG_AP, "STA %d: RSSI: %d", i, sta_info.rssi);
    }
}


void wifi_init(void) {
   // s_wifi_event_group = xEventGroupCreate();

    char saved_ssid[32] = {0};
    char saved_pass[64] = {0};
    uint8_t saved_bssid[6] = {0};
    size_t ssid_len = sizeof(saved_ssid);
    size_t pass_len = sizeof(saved_pass);

    esp_err_t err = read_wifi_credentials_from_nvs1(saved_ssid, &ssid_len, 
                                                    saved_pass, &pass_len, 
                                                    saved_bssid);

    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();// QUAN TRỌNG

    sta_netif = esp_netif_create_default_wifi_sta();// QUAN TRỌNG



    // WiFi init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //start_wifi_events();


    

    // Cấu hình WiFi
    wifi_config_t wifi_config = {0};
    
    if (err == ESP_OK && strlen(saved_ssid) > 0) {
        ESP_LOGI("reconnect", "Found saved WiFi: %s", saved_ssid);
        strncpy((char *)wifi_config.sta.ssid, saved_ssid, sizeof(wifi_config.sta.ssid) - 1);
        strncpy((char *)wifi_config.sta.password, saved_pass, sizeof(wifi_config.sta.password) - 1);
        
        
    } 
    
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
 //   wifi_config.sta.pmf_cfg.capable = true;
 //   wifi_config.sta.pmf_cfg.required = false;

 //if (no_wifi==1){

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    start_wifi_events();  // Start handling Wi-Fi events
    
    ESP_ERROR_CHECK( esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_connect());//

    ESP_LOGI("reconnect", "WiFi init complete, waiting for connection...");
    wifi_wait_connect(); //
    
 }
    //mqtt_start();//
//}

// Set DNS address for the SoftAP mode
void wifi_ap_set_dns_addr(esp_netif_t *sta_netif, esp_netif_t *ap_netif)
{
    esp_netif_dns_info_t dns;

    // Get DNS information from the STA network interface
    esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns);

    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    
    // Stop the DHCP server temporarily
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(ap_netif));

    // Set DNS address for the AP network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns));

    // Restart the DHCP server
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(ap_netif));
}

// Wi-Fi task to handle scanning, station, and AP modes
void wifi_task(void *arg)
{   
   

    wifi_init();  // Initialize the Wi-Fi


    ///////



    static uint8_t connection_num = 0;  // Variable to track the number of connected stations

    while (1)
    {   
        //
       
       
        // Scan Wi-Fi networks if the scan flag is set
        if (WIFI_SCAN_FLAG)
        {
            wifi_scan();
            WIFI_SCAN_FLAG = false;
        }

        // Connect to a Wi-Fi network if the station flag is set
        if (WIFI_STA_FLAG)
        {   
            
            WIFI_STA_FLAG = false;
            
            waveahre_rgb_lcd_set_pclk(12 * 1000 * 1000);  // Set pixel clock for the LCD
            vTaskDelay(20);  // Delay for a short while
     //         if (found_saved_ap)///// khi reset lại/ switch on thì kết nối lại với wifi được lưu trong nvs
     //       {/////
      //         found_saved_ap=false; // dừng kết nối cho các lần sau cho tới khi reset/swtich on 
      //         wifi_sta_init((uint8_t*)saved_ssid, (uint8_t*)saved_password, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);        
                
      //       }/////// 
        //      else {// kết nối tới wifi chọn thủ công
            //wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,NULL);  // Initialize Wi-Fi as STA and connect
            wifi_sta_init(ap_info[wifi_index].ssid, wifi_pwd, ap_info[wifi_index].authmode,ap_info[wifi_index].bssid);  // Initialize Wi-Fi as STA and connect//
         //    }//
            waveahre_rgb_lcd_set_pclk(EXAMPLE_LCD_PIXEL_CLOCK_HZ);  // Restore original pixel clock


     
            lv_timer_t *t = lv_timer_create(wifi_connection_cb, 100, NULL);  // Update UI every 100ms
            
            lv_timer_set_repeat_count(t, 1);  // Run only once

            
            
        }

        // Handle the SoftAP mode if the flag is set
        if (WIFI_AP_FLAG)
        {
            esp_err_t ret = esp_wifi_ap_get_sta_list(&sta_list);  // Get the list of connected stations
            if (ret == ESP_OK)
            {
                // If the number of connected stations has changed, update the UI
                if (connection_num != sta_list.num)
                {
                    lv_timer_t *t = lv_timer_create(wifi_ap_cb, 100, NULL);  // Update UI every 100ms
                    lv_timer_set_repeat_count(t, 1);  // Run only once
                    connection_num = sta_list.num;  // Update connection number
                }
            }
            else
            {
                ESP_LOGE(TAG_AP, "Failed to get STA list");
            }
        }

        vTaskDelay(10);
    }
} 



