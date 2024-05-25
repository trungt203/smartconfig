#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_netif.h"
#include "esp_smartconfig.h"

#include "Wifi.h"

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initialise_wifi();
    vTaskDelay(5000/ portTICK_PERIOD_MS);

    gpio_set_direction(2,GPIO_MODE_OUTPUT);
    while(1){
        gpio_set_level(2, 1);
        vTaskDelay(1000/ portTICK_PERIOD_MS);
        gpio_set_level(2, 0);
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}
