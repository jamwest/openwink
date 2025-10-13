#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include <math.h>
#include "ble_server.h"

static const char *TAG = "MAIN";

#define LED_STRIP_GPIO GPIO_NUM_48
#define LED_STRIP_LED_COUNT 1

static led_strip_handle_t led_strip = NULL;
static BLEServerManager bleServer;

// HSV to RGB conversion
void hsv_to_rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    float r_temp, g_temp, b_temp;

    if (h >= 0 && h < 60) {
        r_temp = c; g_temp = x; b_temp = 0;
    } else if (h >= 60 && h < 120) {
        r_temp = x; g_temp = c; b_temp = 0;
    } else if (h >= 120 && h < 180) {
        r_temp = 0; g_temp = c; b_temp = x;
    } else if (h >= 180 && h < 240) {
        r_temp = 0; g_temp = x; b_temp = c;
    } else if (h >= 240 && h < 300) {
        r_temp = x; g_temp = 0; b_temp = c;
    } else {
        r_temp = c; g_temp = 0; b_temp = x;
    }

    *r = (uint8_t)((r_temp + m) * 255);
    *g = (uint8_t)((g_temp + m) * 255);
    *b = (uint8_t)((b_temp + m) * 255);
}

void rainbow_task(void *pvParameters)
{
    float hue = 0;
    uint8_t r, g, b;
    
    while (1) {
        // Convert HSV to RGB (hue cycles 0-360, saturation=1, value=0.1 for not too bright)
        hsv_to_rgb(hue, 1.0, 0.1, &r, &g, &b);
        
        // Set the LED color
        led_strip_set_pixel(led_strip, 0, r, g, b);
        led_strip_refresh(led_strip);
        
        // Increment hue for next iteration
        hue += 2.0;  // Adjust speed here (higher = faster)
        if (hue >= 360) {
            hue = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(20));  // Update every 20ms
    }
}

void ble_status_task(void *pvParameters)
{
    while (1) {
        ESP_LOGI(TAG, "BLE Connected: %s", bleServer.isConnected() ? "YES" : "NO");
        vTaskDelay(pdMS_TO_TICKS(3000));  // Log every 3 seconds
    }
}

extern "C" void app_main(void)
{
    // Initialize NVS - required for BLE
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "Starting Headlight Control System");
    
    // Configure LED strip - updated for new API
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = LED_STRIP_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false,
        }
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    
    // Initialize BLE
    bleServer.init("OpenWink");
    bleServer.startAdvertising();
    
    // Create tasks
    xTaskCreate(rainbow_task, "rainbow_task", 4096, NULL, 5, NULL);
    xTaskCreate(ble_status_task, "ble_status_task", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "All systems initialized");
}