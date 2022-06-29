#include "apsta_shared.h"

sapstadata_t svr_data;
extern const char *TAG_APSTA;

void apsta_init()
{
    svr_data.TAG_APSTA = TAG_APSTA;
    svr_data.CONNECTED_BIT = BIT0;
    svr_data.wifi_initialized = false;
    svr_data.ap_netif = NULL;
    svr_data.sta_netif = NULL;
    memset((void *)&svr_data.cfg, 0, sizeof(wifi_init_config_t));
    memset((void *)&svr_data.ap_config, 0, sizeof(wifi_config_t));
    memset((void *)&svr_data.sta_config, 0, sizeof(wifi_config_t));

    svr_data.TAGWS = NULL;
    assert(CREDENTIALS_LEN > 1);
    for (int i = 0; i < NB_WIFI_MAX ; i++)
    {
        svr_data.credentials[i].SSID[0] = ' '; // SSID[0] = ' ';
        svr_data.credentials[i].SSID[1] = 0;
        svr_data.credentials[i].PASS[0] = ' ';
        svr_data.credentials[i].PASS[1] = 0;
    }
    memset((void *)&svr_data.credential, 0, sizeof(httpd_uri_t));
    memset((void *)&svr_data.ws, 0, sizeof(httpd_uri_t));
    svr_data.as_resp_arg = NULL;
    svr_data.fd = 0;
    memset(&svr_data.ws_pkt, 0, sizeof(httpd_ws_frame_t));
    svr_data.trig_resp_arg = NULL;
}

psapstadata_t get_svrdata()
{
    return &svr_data;
}