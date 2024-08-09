#ifndef DTMF_H
#define DTMF_H

#include "driver/gpio.h"
#include "esp_err.h"

esp_err_t dtmf_init(gpio_num_t pin);
void dtmf_play_tone(char key);

#endif // DTMF_H