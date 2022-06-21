#ifndef __APSTA_H__
#define __APSTA_H__

#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <sys/param.h> // cherche dans un rep de l'environnement du compilo
#include "esp_netif.h"
#include "esp_eth.h"
#include <sys/param.h>
#include <esp_http_server.h>

#define CRED_DATA_T uint8_t
#define CRED_NONE ((CRED_DATA_T)(CRED_DATA_T)0)
#define CRED_SSID ((CRED_DATA_T)(CRED_DATA_T)1)
#define CRED_PASS ((CRED_DATA_T)(CRED_DATA_T)2)
#define CRED_COMPLETE (CRED_SSID | CRED_PASS)
#define CRED_MASK_ALL (CRED_SSID | CRED_PASS)
#define CRED_MASK_INV (~(CRED_SSID | CRED_PASS))
#define CREDENTIALS_LEN 65
#define STREND (CREDENTIALS_LEN - 1)

typedef struct
{
    char SSID[CREDENTIALS_LEN];
    char PASS[CREDENTIALS_LEN];
    uint32_t CRC32;
} ts_credentials;

typedef struct
{
    httpd_handle_t hd;
    int fd;
} async_resp_arg;



typedef struct
{
    char *TAG_APSTA;
    EventGroupHandle_t wifi_event_group;
    int CONNECTED_BIT;
    bool wifi_initialized;
    bool wifi_sta_connected;
    bool wifi_sta_available;
    esp_netif_t *ap_netif;
    esp_netif_t *sta_netif;
    wifi_init_config_t cfg;
    wifi_config_t ap_config;
    wifi_config_t sta_config;
    // websocket
    char *TAGWS;
    ts_credentials credentials;
    httpd_uri_t credential;
    httpd_uri_t ws;
    async_resp_arg *as_resp_arg;
    int fd;
    httpd_ws_frame_t ws_pkt;
    async_resp_arg *trig_resp_arg;

    EventGroupHandle_t sync_event_group;

} sapstadata_t;
typedef sapstadata_t *psapstadata_t;

void apsta_init();
psapstadata_t get_svrdata();

#endif