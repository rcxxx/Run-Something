#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <regex.h>

#include <stdio.h>
#include "driver/ledc.h"

/*--- WiFi ---*/
#define WIFI_SSID "HUAWEI-DFP"
#define WIFI_PASS "dianfengpai"
#define MAXIMUM_RETRY  5

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFI_TAG = "wifi station";
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits( s_wifi_event_group,
                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID,WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }
}
/*--- WiFi ---*/

/*--- Socket ---*/
#define UDP_PORT 2333
static const char *UDP_TAG = "UDP Client";
static const char *device_name = "Run-Parrot";

#define TCP_PORT 2334
static const char *TCP_TAG = "TCP Client";

char g_server_ip[16] = {0};

float g_cpu_usage = 0.0;
int g_mem_usage = 0;

void udp_client_task(void *pvParameters) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    while (1)
    {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            ESP_LOGE(UDP_TAG, "Failed to create socket: errno %d", errno);
            continue;
        }

        int broadcast = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0 ) {
            ESP_LOGE(UDP_TAG, "Failed to set broadcast option: errno %d", errno);
            close(sock);
            continue;
        }

        char tx_buffer[128];
        sprintf(tx_buffer, "ESP32 Device:%s", device_name);
        char rx_buffer[128];

        while (1)
        {
            struct sockaddr_in source_addr;
            socklen_t socklen = sizeof(source_addr);
            int err = sendto(sock, tx_buffer, strlen(tx_buffer), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err < 0) {
                ESP_LOGE(UDP_TAG, "Error occurred during sending: errno %d", errno);
                close(sock);
                break;
            } else {
                ESP_LOGI(UDP_TAG, "sending: %s to port: %d", tx_buffer, UDP_PORT);
            }

            struct timeval timeout;
            timeout.tv_sec = 20; // 10s
            timeout.tv_usec = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                ESP_LOGE(UDP_TAG, "Failed to set timeout option: errno %d", errno);
                break;
            }
            
            memset(rx_buffer, 0, sizeof(rx_buffer));
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ESP_LOGE(UDP_TAG, "recvfrom timeout\n");
                    break;
                } else {
                    ESP_LOGE(UDP_TAG, "Error occurred during receiving: errno %d", errno);
                    break;
                }
            } else {
                rx_buffer[len] = 0;
                ESP_LOGI(UDP_TAG, "Received %d bytes from %s:%d", len, inet_ntoa(source_addr.sin_addr), ntohs(source_addr.sin_port));
                ESP_LOGI(UDP_TAG, "%s", rx_buffer);
                // Save server IP address
                inet_ntoa_r(source_addr.sin_addr, g_server_ip, sizeof(g_server_ip));
                ESP_LOGI(UDP_TAG, "Get server ip: %s", g_server_ip);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    ESP_LOGE(UDP_TAG, "Failed to set timeout option: errno %d", errno);
                    break;
                }
            }

            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
        
        close(sock);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void tcp_client_task(void *pvParameters) {
    char rx_buffer[128];
    
    while (1)
    {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = inet_addr(g_server_ip);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(TCP_PORT);
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            ESP_LOGE(TCP_TAG, "Failed to create TCP socket: errno %d", errno);
            continue;
        }

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TCP_TAG, "Failed to connect TCP server: \"%s:%d\" errno: %d", inet_ntoa(dest_addr.sin_addr), ntohs(dest_addr.sin_port), errno);
            ESP_LOGI(TCP_TAG, "Retry...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        while (1)
        {
            struct timeval timeout;
            timeout.tv_sec = 10; // 10s
            timeout.tv_usec = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                ESP_LOGE(TCP_TAG, "Failed to set timeout option: errno %d", errno);
                continue;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ESP_LOGE(TCP_TAG, "recv timeout\n");
                    continue;
                } else {
                    ESP_LOGE(TCP_TAG, "recv failed: errno %d", errno);
                    break;
                }
            } else {
                rx_buffer[len] = 0;
                ESP_LOGI(TCP_TAG, "Received %d bytes from %s", len, inet_ntoa(dest_addr.sin_addr));
                ESP_LOGI(TCP_TAG, "%s", rx_buffer);
                timeout.tv_sec = 0; // 10s
                timeout.tv_usec = 0;
                if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    ESP_LOGE(TCP_TAG, "Failed to set timeout option: errno %d", errno);
                    continue;
                }

                regex_t regex;
                regmatch_t match[3];
                char cpu_str[16];
                char mem_str[16];

                int ret = regcomp(&regex, "CPU Usage: ([0-9]+\\.[0-9]+)%  Memory Usage: ([0-9]+\\.[0-9]+)%", REG_EXTENDED);
                if (ret != 0) {
                    ESP_LOGE(TCP_TAG, "Failed to compile regex");
                    continue;
                }

                // 正则匹配
                ret = regexec(&regex, rx_buffer, 3, match, 0);
                if (ret != 0) {
                    ESP_LOGE(TCP_TAG, "Failed to match regex");
                    regfree(&regex);
                    continue;
                }

                // 提取数字
                memcpy(cpu_str, rx_buffer + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
                cpu_str[match[1].rm_eo - match[1].rm_so] = '\0';
                g_cpu_usage = strtof(cpu_str, NULL);

                memcpy(mem_str, rx_buffer + match[2].rm_so, match[2].rm_eo - match[2].rm_so);
                mem_str[match[2].rm_eo - match[2].rm_so] = '\0';
                g_mem_usage = atoi(mem_str);

                ESP_LOGI(TCP_TAG, "CPU Usage: %.2f%%  Memory Usage: %d%%", g_cpu_usage, g_mem_usage);
            }
            
            // vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        
        if (sock != -1) {
            ESP_LOGE(TCP_TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }

        close(sock);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}
/*--- Socket ---*/

/*--- PWM ---*/
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (9) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_1

void pwm_task(void *arg) {
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 5000,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    uint32_t pwm_duty = 2000;

    while(1) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, pwm_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        pwm_duty = (uint32_t)(2000 + (6000.0f * g_cpu_usage * 0.01f));
    }
}
/*--- PWM ---*/

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL);
    
    xTaskCreate(tcp_client_task, "udp_client", 4096, NULL, 5, NULL);

    xTaskCreate(pwm_task, "pwm_task", 2048, NULL, 5, NULL);
}
