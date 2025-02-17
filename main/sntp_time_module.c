#include "sntp_time_module.h"
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG_TIME = "sntp_sync"; // Define TAG here, and rename to sntp_sync for clarity

// Function to initialize and start the SNTP client
void initialize_sntp(void) {
    ESP_LOGI(TAG_TIME, "Initializing SNTP");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}


// Function to sync time
void sync_time(void) {
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    initialize_sntp();

    // Wait for time to be set
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG_TIME, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    localtime_r(&now, &timeinfo);

    // If time sync failed, set a default time
    if (timeinfo.tm_year < (2024 - 1900)) {
        ESP_LOGI(TAG_TIME, "Time sync failed, setting default time");
        timeinfo.tm_year = 2024 - 1900;
        timeinfo.tm_mon = 8;  // September (0-based)
        timeinfo.tm_mday = 29;
        timeinfo.tm_hour = 20;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
        struct timeval tv = { .tv_sec = mktime(&timeinfo) };
        settimeofday(&tv, NULL);
    }

    setenv("TZ", "VET4", 1);  // Venezuela Time Zone
    tzset();

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG_TIME, "The current date/time is: %s", strftime_buf);
}

// Function to get current time
void get_current_time(struct tm *timeinfo) {
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
}