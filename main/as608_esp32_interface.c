// as608_esp32_interface.c

#include "driver_as608_interface.h"
#include "driver/uart.h"
#include "esp_log.h"

#define AS608_UART_NUM UART_NUM_1
#define AS608_TXD_PIN 17
#define AS608_RXD_PIN 18
#define AS608_UART_BAUD_RATE 57600
#define AS608_BUF_SIZE (1024)

static const char *TAG = "AS608_INTERFACE";

uint8_t as608_interface_uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = AS608_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(AS608_UART_NUM, AS608_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(AS608_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(AS608_UART_NUM, AS608_TXD_PIN, AS608_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    return 0;
}

uint8_t as608_interface_uart_deinit(void)
{
    ESP_ERROR_CHECK(uart_driver_delete(AS608_UART_NUM));
    return 0;
}

uint16_t as608_interface_uart_read(uint8_t *buf, uint16_t len)
{
    int length = uart_read_bytes(AS608_UART_NUM, buf, len, 1000 / portTICK_PERIOD_MS);
    return (uint16_t)length;
}

uint8_t as608_interface_uart_write(uint8_t *buf, uint16_t len)
{
    int length = uart_write_bytes(AS608_UART_NUM, (const char*)buf, len);
    return (length == len) ? 0 : 1;
}

uint8_t as608_interface_uart_flush(void)
{
    ESP_ERROR_CHECK(uart_flush(AS608_UART_NUM));
    return 0;
}

void as608_interface_delay_ms(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void as608_interface_debug_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    ESP_LOGI(TAG, "%s", buf);
    va_end(args);
}
