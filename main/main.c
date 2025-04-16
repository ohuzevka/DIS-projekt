/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"

#include "bsp/esp-bsp.h"
#include "es8311.h"
#include "led_strip.h"
#include "lvgl.h"
#include "disp.h"

/* Buffer for reading/writing to I2S driver. Same length as SPIFFS buffer and I2S buffer, for optimal read/write performance.
   Recording audio data path:
   I2S peripheral -> I2S buffer (DMA) -> App buffer (RAM) -> SPIFFS buffer -> External SPI Flash.
   Vice versa for playback. */
#define BUFFER_SIZE     (1024)
#define SAMPLE_RATE     (22050)
#define DEFAULT_VOLUME  (50)
/* The recording will be RECORDING_LENGTH * BUFFER_SIZE long (in bytes)
   With sampling frequency 22050 Hz and 16bit mono resolution it equals to ~3.715 seconds */
// #define RECORDING_LENGTH (160)

/* Globals */
static const char *TAG = "example";
static button_handle_t audio_button[BSP_BUTTON_NUM] = {};
static QueueHandle_t audio_button_q = NULL;
static led_strip_handle_t rgb_led = NULL;
static i2s_chan_handle_t i2s_tx_chan;
static i2s_chan_handle_t i2s_rx_chan;


enum ClockStates {
    Setup,
    Pause,
    Playing,
    Timeout
};
enum Players { Player1, Player2 };

enum ClockStates clock_state = Setup;
enum Players active_player = Player1;
TaskHandle_t refresh_diaplay_handle;
TaskHandle_t play_audio_handle;
unsigned int set_time = 60;         // Starting time [s]
unsigned int time_step = 10;        // Time to add or subtract with +/- button press [s]
unsigned int player1_time = 60;      // Remaining time of player1 [s]
unsigned int player2_time = 60;      // Remaining time of player2 [s]


static void btn_handler(void *arg, void *arg2)
{
    for (uint8_t i = 0; i < BSP_BUTTON_NUM; i++) {
        if ((button_handle_t)arg == audio_button[i]) {
            xQueueSend(audio_button_q, &i, 0);
            break;
        }
    }
}

// Very simple WAV header, ignores most fields
typedef struct __attribute__((packed))
{
    uint8_t ignore_0[22];
    uint16_t num_channels;
    uint32_t sample_rate;
    uint8_t ignore_1[6];
    uint16_t bits_per_sample;
    uint8_t ignore_2[4];
    uint32_t data_size;
    uint8_t data[];
} dumb_wav_header_t;

static esp_err_t spiffs_init(void)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "Initializing SPIFFS");

    const esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    /*!< Use settings defined above to initialize and mount SPIFFS filesystem. */
    /*!< Note: esp_vfs_spiffs_register is an all-in-one convenience function. */
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }

        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

void play_audio() {
    /* Create and configure ES8311 I2C driver */
    es8311_handle_t es8311_dev = es8311_create(BSP_I2C_NUM, ES8311_ADDRRES_0);
    const es8311_clock_config_t clk_cfg = BSP_ES8311_SCLK_CONFIG(SAMPLE_RATE);
    es8311_init(es8311_dev, &clk_cfg, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    es8311_voice_volume_set(es8311_dev, DEFAULT_VOLUME, NULL);

    /* Configure I2S peripheral and Power Amplifier */
    bsp_audio_init(NULL, &i2s_tx_chan, &i2s_rx_chan);
    bsp_audio_poweramp_enable(true);

    /* Pointer to a file that is going to be played */
    const char music_filename[] = "/spiffs/16bit_mono_22_05khz.wav";
    const char *play_filename = music_filename;

    while (1) {
        vTaskSuspend( NULL );   // Task suspends itself, Waits until resuming
        
        int16_t *wav_bytes = malloc(BUFFER_SIZE);
        assert(wav_bytes != NULL);

        /* Open WAV file */
        ESP_LOGI(TAG, "Playing file %s", play_filename);
        FILE *play_file = fopen(play_filename, "rb");
        if (play_file == NULL) {
            ESP_LOGW(TAG, "%s file does not exist!", play_filename);
            return;
        }

        /* Read WAV header file */
        dumb_wav_header_t wav_header;
        if (fread((void *)&wav_header, 1, sizeof(wav_header), play_file) != sizeof(wav_header)) {
            ESP_LOGW(TAG, "Error in reading file");
            return;
        }
        ESP_LOGI(TAG, "Number of channels: %d", wav_header.num_channels);
        ESP_LOGI(TAG, "Bits per sample: %d", wav_header.bits_per_sample);
        ESP_LOGI(TAG, "Sample rate: %d", wav_header.sample_rate);
        ESP_LOGI(TAG, "Data size: %d", wav_header.data_size);

        uint32_t bytes_send_to_i2s = 0;
        while (bytes_send_to_i2s < wav_header.data_size) {
            /* Get data from SPIFFS */
            size_t bytes_read_from_spiffs = fread(wav_bytes, 1, BUFFER_SIZE, play_file);

            /* Send it to I2S */
            size_t i2s_bytes_written;
            ESP_ERROR_CHECK(i2s_channel_write(i2s_tx_chan, wav_bytes, bytes_read_from_spiffs, &i2s_bytes_written, pdMS_TO_TICKS(500)));
            bytes_send_to_i2s += i2s_bytes_written;
        }

        fclose(play_file);
        free(wav_bytes);
    }
}

void btn_actions() 
{   
    while (1) {

    uint8_t btn_index = 0;
    if (xQueueReceive(audio_button_q, &btn_index, portMAX_DELAY) != pdTRUE) {
        continue;
    }
            
    switch (btn_index) {
        case BSP_BUTTON_REC: {      // Player 2 finish turn
            if (clock_state == Timeout) {
                clock_state = Setup;
                player1_time = set_time;
                player2_time = set_time;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            else if (clock_state == Playing) {
                active_player = Player1;
                disp_set_P1_cb(true);
                disp_set_P2_cb(false);
            }
            else if (clock_state == Setup || clock_state == Pause) {
                active_player = Player1;
                clock_state = Playing;
                disp_set_P1_cb(true);
                disp_set_P2_cb(false);
            }
            break;
        }
        case BSP_BUTTON_MODE: {     // Increase starting time
            if (clock_state == Setup) {
                set_time += time_step;
                player1_time = set_time;
                player2_time = set_time;
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            else if (clock_state == Timeout) {
                clock_state = Setup;
                player1_time = set_time;
                player2_time = set_time;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            break;
        }
        case BSP_BUTTON_PLAY: {     // Play / pause
            if (clock_state == Setup || clock_state == Pause) {
                clock_state = Playing;
                if (active_player == Player1) {
                    disp_set_P1_cb(true);
                    disp_set_P2_cb(false);
                }
                else {
                    disp_set_P1_cb(false);
                    disp_set_P2_cb(true);
                }
            } 
            else if (clock_state == Playing) {
                clock_state = Pause;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
            }
            else if (clock_state == Timeout) {
                clock_state = Setup;
                player1_time = set_time;
                player2_time = set_time;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            break;
        }
        case BSP_BUTTON_SET: {      // Reset to starting time
            clock_state = Setup;
            player1_time = set_time;
            player2_time = set_time;
            disp_set_P1_cb(false);
            disp_set_P2_cb(false);
            vTaskResume(refresh_diaplay_handle);    // Refresh display
            break;
        }
        case BSP_BUTTON_VOLDOWN: {  // Decrease starting time
            if (clock_state == Setup) {
                if (set_time - time_step != 0) {
                    set_time -= time_step;
                }
                player1_time = set_time;
                player2_time = set_time;
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            else if (clock_state == Timeout) {
                clock_state = Setup;
                player1_time = set_time;
                player2_time = set_time;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            break;
        }
        case BSP_BUTTON_VOLUP: {    // Player 1 finish turn
            if (clock_state == Timeout) {
                clock_state = Setup;
                player1_time = set_time;
                player2_time = set_time;
                disp_set_P1_cb(false);
                disp_set_P2_cb(false);
                vTaskResume(refresh_diaplay_handle);    // Refresh display
            }
            else if (clock_state == Playing) {
                active_player = Player2;
                disp_set_P1_cb(false);
                disp_set_P2_cb(true);
            }
            else if (clock_state == Setup || clock_state == Pause) {
                active_player = Player2;
                clock_state = Playing;
                disp_set_P1_cb(false);
                disp_set_P2_cb(true);
            }
            break;
        }
        default:
            ESP_LOGW(TAG, "Button index out of range");
    }
}
}

// clock tick executed every second
void clock_tick()
{
    TickType_t xLastWakeTime = xTaskGetTickCount();;
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);    // clock period in ms

    while(1) {
        vTaskDelayUntil( &xLastWakeTime, xPeriod );

        if (clock_state == Playing) {
            if (active_player == Player1) {
                player1_time--;
                if (player1_time == 0) {
                    clock_state = Timeout;
                    vTaskResume(play_audio_handle);     // Play audio
                }
            }
            else if (active_player == Player2) {
                player2_time--;
                if (player2_time == 0) {
                    clock_state = Timeout;
                    vTaskResume(play_audio_handle);     // Play audio
                }
            }
            vTaskResume(refresh_diaplay_handle);        // Refresh display
        }
    }
}

void refresh_display()
{
    while(1) {
        vTaskSuspend( NULL );   // Task suspends itself, Waits until resuming
        disp_set_clock1(set_time, player1_time);
        disp_set_clock2(set_time, player2_time);
    }
}

void led() {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (clock_state == Playing) {
            if (active_player == Player1) {
                led_strip_set_pixel(rgb_led, 0, 100, 0, 0);
            }
            else {
                led_strip_set_pixel(rgb_led, 0, 0, 0, 100);
            }
        }
        else {
            led_strip_set_pixel(rgb_led, 0, 0, 0, 0);
        }
        led_strip_refresh(rgb_led);
    }
}


void app_main(void)
{
    /* Init board peripherals */
    bsp_i2c_init(); // Used by ES8311 driver
    ESP_ERROR_CHECK(spiffs_init());

    /* Configure RGB LED */
    const led_strip_config_t rgb_config = {
        .strip_gpio_num = BSP_LEDSTRIP_IO,
        .max_leds = 1,
    };
    const led_strip_rmt_config_t rmt_config = {0};
    led_strip_new_rmt_device(&rgb_config, &rmt_config, &rgb_led);
    assert(rgb_led != NULL);
    ESP_ERROR_CHECK(led_strip_clear(rgb_led));

    /* Needed from random RGB LED color generation */
    time_t t;
    srand((unsigned) time(&t));

    /* Create FreeRTOS tasks and queues */
    audio_button_q = xQueueCreate(10, sizeof(uint8_t));
    assert (audio_button_q != NULL);

    // clock tick executed every second
    xTaskCreate(clock_tick, "clock_tick", 4096, NULL, 1, NULL);
    xTaskCreate(refresh_display, "refresh display", 4096, NULL, 7, &refresh_diaplay_handle);
    xTaskCreate(play_audio, "play_audio", 4096, NULL, 7, &play_audio_handle);
    xTaskCreate(btn_actions, "btn_actions", 4096, NULL, 6, NULL);
    xTaskCreate(led, "led", 4096, NULL, 6, NULL);

    /* Init audio buttons */
    for (int i = 0; i < BSP_BUTTON_NUM; i++) {
        audio_button[i] = iot_button_create(&bsp_button_config[i]);
        assert(audio_button[i] != NULL);
        ESP_ERROR_CHECK(iot_button_register_cb(audio_button[i], BUTTON_PRESS_DOWN, btn_handler, NULL));
    }

    lv_disp_t *disp;
    disp = bsp_display_start(); // Start LVGL and LCD driver
    bsp_display_rotate(disp, LV_DISP_ROT_90);
    disp_init();         // Create LVGL screen and widgets
    // disp_set_volume(DEFAULT_VOLUME);
}
