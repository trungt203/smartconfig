#include "Wifi.h"

char *ss_str;
char *psw_str;
uint8_t retry = 0;
esp_err_t err;
const char* ssid_default = "TrungPhat";
const char* pass_default = "30090610";

static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "smartconfig_example";

esp_err_t ReadSSID_PASS();
esp_err_t WriteSSID_PASS(uint8_t *ssid, uint8_t *pass);
void initialise_wifi();
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void smartconfig_example_task(void * parm);

esp_err_t ReadSSID_PASS(void){
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if(err != ESP_OK) return err;

        //SSID-------------------------------
        printf("Reading ssid from NVS ... ");
        size_t required_size = 0;
        err = nvs_get_str(my_handle, "ssid", NULL, &required_size);
        if(required_size == 0){
            ss_str = ssid_default;
            ESP_LOGI(TAG, "SSID_Default");
        } else {

            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
                
            char *ssid = malloc(required_size);
            err = nvs_get_str(my_handle, "ssid", ssid, &required_size); 
            if(err == ESP_OK){
                ESP_LOGI(TAG, "SSID_READ:%s", ssid);
            } else free(ssid);
            ss_str = ssid;

        }
        
        //PASS---------------------------
        printf("Reading pass from NVS ... ");
        err = nvs_get_str(my_handle, "pass", NULL, &required_size);
        if(required_size == 0){
            psw_str = pass_default;
            ESP_LOGI(TAG, "PASS_Default");
        } else {

            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
                
            char *pass = malloc(required_size);
            err = nvs_get_str(my_handle, "pass", pass, &required_size); 
            if(err == ESP_OK){
                ESP_LOGI(TAG, "PASS_READ:%s", pass);
            } else free(pass);
            psw_str = pass;

        }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t WriteSSID_PASS(uint8_t *ssid, uint8_t *pass){
    char* ssid_str = (char *)ssid;
    char* pass_str = (char *)pass;
    if((strcmp(ssid_str, ssid_default) == 0) || (strcmp(pass_str, pass_default) == 0)) {
        esp_err_t err;
        nvs_handle_t my_handle;
        err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if(err != ESP_OK) return err;

        printf("Updating ssid in NVS ... ");
        if(strcmp(ssid_str, ssid_default) != 0){
            err = nvs_set_str(my_handle, "ssid", ssid_str );
            printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        }
        
        printf("Updating password in NVS ... ");
        if(strcmp(pass_str, pass_default) != 0){
            err = nvs_set_str(my_handle, "pass", pass_str );
            printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        }
        
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }
    return ESP_OK;
}

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("Connecting\n");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
        printf("Connected\n");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if(retry < 3){
            wifi_config_t wifi_config = {
                .sta = {
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                    .sae_pwe_h2e = WIFI_AUTH_WPA2_PSK,
                    .sae_h2e_identifier = "",
                },
            };
            strcpy((char *)wifi_config.sta.ssid, ss_str);
            strcpy((char *)wifi_config.sta.password, psw_str);

            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            ESP_ERROR_CHECK(esp_wifi_connect());
            
            retry++;
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            printf("Reconnecting\n");
        } else{
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);
        WriteSSID_PASS(ssid, password);

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void initialise_wifi(){
    esp_err_t err;

    err = ReadSSID_PASS();
    if (err != ESP_OK) printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));

    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    
    esp_event_loop_create_default();     
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL) );

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WIFI_AUTH_WPA2_PSK,
            .sae_h2e_identifier = "",
        },
    };
    strcpy((char *)wifi_config.sta.ssid, ss_str);
    strcpy((char *)wifi_config.sta.password, psw_str);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);   
    esp_wifi_start();

}

void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}