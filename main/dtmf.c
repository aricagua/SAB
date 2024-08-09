#include "dtmf.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define DTMF_TAG "DTMF"

// DTMF frequencies
const uint16_t row_freqs[4] = {697, 770, 852, 941};
const uint16_t col_freqs[4] = {1209, 1336, 1477, 1633};

// PWM configuration
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // 13 bit duty resolution
#define LEDC_FREQUENCY          5000              // 5000 Hz PWM signal frequency
#define LEDC_CHANNEL_ROW        LEDC_CHANNEL_0
#define LEDC_CHANNEL_COL        LEDC_CHANNEL_1

static gpio_num_t buzzer_pin;

esp_err_t dtmf_init(gpio_num_t pin) {
    buzzer_pin = pin;

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel_row = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_ROW,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = buzzer_pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_row));

    ledc_channel_config_t ledc_channel_col = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_COL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = buzzer_pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_col));

    return ESP_OK;
}

void dtmf_play_tone(char key) {
    int row, col;
    switch (key) {
        case '1': row = 0; col = 0; break;
        case '2': row = 0; col = 1; break;
        case '3': row = 0; col = 2; break;
        case 'A': row = 0; col = 3; break;
        case '4': row = 1; col = 0; break;
        case '5': row = 1; col = 1; break;
        case '6': row = 1; col = 2; break;
        case 'B': row = 1; col = 3; break;
        case '7': row = 2; col = 0; break;
        case '8': row = 2; col = 1; break;
        case '9': row = 2; col = 2; break;
        case 'C': row = 2; col = 3; break;
        case '*': row = 3; col = 0; break;
        case '0': row = 3; col = 1; break;
        case '#': row = 3; col = 2; break;
        case 'D': row = 3; col = 3; break;
        default: return; // Invalid key
    }

    ESP_LOGI(DTMF_TAG, "Playing DTMF tone for key: %c", key);

    // Set frequencies
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, row_freqs[row]);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_ROW, 4096); // 50% duty cycle
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_ROW);

    ledc_set_freq(LEDC_MODE, LEDC_TIMER, col_freqs[col]);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_COL, 4096); // 50% duty cycle
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_COL);

    // Play tone for 200ms
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Stop tone
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_ROW, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_ROW);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_COL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_COL);
}