#include "apsta.h"
#include "websocket.h"
#include "apsta_shared.h"

const char *TAG_APSTA = "AP_STATION";

void event_handler(void *arg, esp_event_base_t event_base,
				   int32_t event_id, void *event_data)
{
	if (event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		printf(" \n Station disconnected \n");
		get_svrdata()->wifi_sta_connected = false;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		// ESP_LOGI(get_svrdata()->TAG_APSTA, "WIFI_EVENT_STA_DISCONNECTED");

		// esp_wifi_connect
		/*
		Connect the ESP32 WiFi station to the AP.

		Attention
			1. This API only impact WIFI_MODE_STA or WIFI_MODE_APSTA mode

			2. If the ESP32 is connected to an AP, call esp_wifi_disconnect to disconnect.

			3. The scanning triggered by esp_wifi_start_scan() will not be effective until connection
			between ESP32 and the AP is established. If ESP32 is scanning and connecting at the same time,
			ESP32 will abort scanning and return a warning message and error number ESP_ERR_WIFI_STATE.
			If you want to do reconnection after ESP32 received disconnect event, remember to add the maximum
			retry time, otherwise the called scan will not work. This is especially true when the AP doesn’t exist,
			and you still try reconnection after ESP32 received disconnect event with the reason code WIFI_REASON_NO_AP_FOUND.

		Returns
			ESP_OK: succeed
			ESP_ERR_WIFI_NOT_INIT: WiFi is not initialized by esp_wifi_init
			ESP_ERR_WIFI_NOT_STARTED: WiFi is not started by esp_wifi_start
			ESP_ERR_WIFI_CONN: WiFi internal error, station or soft-AP control block wrong
			ESP_ERR_WIFI_SSID: SSID of AP which station connects is invalid
		*/
		esp_wifi_connect();
		xEventGroupClearBits(get_svrdata()->wifi_event_group, get_svrdata()->CONNECTED_BIT);
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ESP_LOGI(get_svrdata()->TAG_APSTA, "IP_EVENT_STA_GOT_IP");
		xEventGroupSetBits(get_svrdata()->wifi_event_group, get_svrdata()->CONNECTED_BIT); // Clear bits (flags) within an RTOS event group.
		printf("\n #################### Station connected #################### \n");
		get_svrdata()->wifi_sta_connected = true;
	}
}

void initialise_wifi(void)
{
	esp_log_level_set("wifi", ESP_LOG_WARN);

	if (get_svrdata()->wifi_initialized)
	{
		return;
	}
	ESP_ERROR_CHECK(esp_netif_init());
	get_svrdata()->wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	// esp_netif_t
	/*
	Creates an instance of new esp-netif object based on provided config.

	Parameters
		esp_netif_config – pointer esp-netif configuration

	Returns
		pointer to esp-netif object on success
		NULL otherwise
	*/
	// esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
	// assert(ap_netif);
	// esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	// assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	get_svrdata()->cfg = cfg;

	get_svrdata()->ap_netif = esp_netif_create_default_wifi_ap();
	get_svrdata()->sta_netif = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK(esp_wifi_init(&(get_svrdata()->cfg)));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
	// ESP_ERROR_CHECK( esp_wifi_start() );

	get_svrdata()->wifi_initialized = true;
}

bool wifi_apsta(int timeout_ms)
{

	strcpy((char *)(get_svrdata()->ap_config.ap.ssid), "Batconnect");
	strcpy((char *)(get_svrdata()->ap_config.ap.password), "password");
	get_svrdata()->ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	get_svrdata()->ap_config.ap.ssid_len = strlen("Batconnect");
	get_svrdata()->ap_config.ap.max_connection = CONFIG_AP_MAX_STA_CONN;
	get_svrdata()->ap_config.ap.channel = CONFIG_AP_WIFI_CHANNEL;

	if (strlen("CONFIG_AP_WIFI_PASSWORD") == 0)
	{
		get_svrdata()->ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	/************* LECTURE DE LA FLASH ************/
	// Initialize NVS
	esp_err_t err2 = nvs_flash_init();
	if (err2 == ESP_ERR_NVS_NO_FREE_PAGES || err2 == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err2 = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err2);

	// Open
	printf("\n");
	printf("Opening Non-Volatile Storage (NVS) handle... ");
	nvs_handle_t my_handle;
	err2 = nvs_open("storage", NVS_READWRITE, &my_handle);
	if (err2 != ESP_OK)
	{
		printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err2));
	}
	else
	{
		printf("nvs_open Done\n");

		// Read
		printf("Reading restart counter from NVS ... \n\n");

		// char msg_get[STRLN]="                ";
		ts_credentials my_struct;
		strcpy(my_struct.SSID, "  ");
		strcpy(my_struct.PASS, "  ");

		size_t s = sizeof(ts_credentials) / sizeof(uint8_t);
		err2 = nvs_get_blob(my_handle, "ssid", (uint8_t *)&my_struct,
							&s);

		my_struct.SSID[STREND] = 0; // au cas ou
		my_struct.PASS[STREND] = 0; // au cas ou

		printf("-->get SSID = %s\n", my_struct.SSID);
		printf("-->get PASS = %s \n\n", my_struct.PASS);

		switch (err2)
		{
		case ESP_OK:
			// printf("ESP_OK nvs_set_str \n");
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			printf("The value is not initialized yet!\n");
			break;
		default:
			printf("Error (%s) reading!\n", esp_err_to_name(err2));
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
		err2 = nvs_commit(my_handle);
		printf((err2 != ESP_OK) ? "Failed!\n" : "nvs_commit  Done\n");
		// Close
		vTaskDelay(100 / portTICK_PERIOD_MS);
		nvs_close(my_handle);

		/**********************************************/

		/************* MISE A JOUR DE LA CONFIG	*************/
		printf("\n\n\n MISE A JOUR DE LA CONFIG");
		strcpy((char *)(get_svrdata()->sta_config.sta.ssid), my_struct.SSID);
		strcpy((char *)(get_svrdata()->sta_config.sta.password), my_struct.PASS);
	}

	// printf("\n ICI TEST !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
	// printf((char *)(get_svrdata()->sta_config.sta.ssid)); // verification de la maj du parametre ssid
	// printf("\n");
	// printf((char *)(get_svrdata()->sta_config.sta.password)); // verification de la maj du parametre pwd

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &get_svrdata()->ap_config));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &get_svrdata()->sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(get_svrdata()->TAG_APSTA,
			 "WIFI_MODE_AP started. SSID:%s password:%s channel:%d",
			 (char *)(get_svrdata()->ap_config.ap.ssid),
			 (char *)(get_svrdata()->ap_config.ap.password),
			 CONFIG_AP_WIFI_CHANNEL);

	ESP_ERROR_CHECK(esp_wifi_connect());
	int bits = xEventGroupWaitBits(get_svrdata()->wifi_event_group, get_svrdata()->CONNECTED_BIT,
								   pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
	ESP_LOGI(get_svrdata()->TAG_APSTA, "bits=%x", bits);

	// if (bits) {
	// 	printf("\n ############################################ \n");
	// 	printf("\n %s \n", get_svrdata()->credentials.SSID);

	// 	ESP_LOGI(get_svrdata()->TAG_APSTA, "WIFI_MODE_STA connected. SSID:%s password:%s",
	// 	 		 get_svrdata()->credentials.SSID, get_svrdata()->credentials.PASS);
	// 	// ESP_LOGI(get_svrdata()->TAG_APSTA, "WIFI_MODE_STA connected. SSID:%s password:%s",
	// 	// 		 get_svrdata()->SSID, get_svrdata()->PWD);
	// } else {

	ESP_LOGI(get_svrdata()->TAG_APSTA, "WIFI_MODE_STA connecting. SSID:%s password:%s",
			 get_svrdata()->credentials.SSID, get_svrdata()->credentials.PASS);
	// ESP_LOGI(get_svrdata()->TAG_APSTA, "WIFI_MODE_STA can't connected. SSID:%s password:%s",
	// 		 get_svrdata()->SSID, get_svrdata()->PWD);
	//}
	return (bits & get_svrdata()->CONNECTED_BIT) != 0;
}

void scann_wifi_around()
{
	printf("1 \n");
	ESP_ERROR_CHECK(nvs_flash_init());
	printf("2 \n");

	tcpip_adapter_init();
	printf("3 \n");

	wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
	printf("4 \n");
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
	printf("5 \n");
	ESP_ERROR_CHECK(esp_wifi_start()); // starts wifi usage
	printf("6 \n");
	// configure and run the scan process in blocking mode
	wifi_scan_config_t scan_config = {
		.ssid = 0,
		.bssid = 0,
		.channel = 0,
		.show_hidden = true};
	printf("Start scanning... \n");
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
	// printf(" completed!\n");
	// get the list of APs found in the last scan
	uint16_t ap_num;
	wifi_ap_record_t ap_records[20];
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));
	// print the list
	printf("Found %d access points:\n", ap_num);

	// printf("               SSID              | Channel | RSSI |   MAC \n\n");
	// printf("----------------------------------------------------------------\n");
	for (int i = 0; i < ap_num; i++)
	{
		printf("%32s | %7d | %4d   %2x:%2x:%2x:%2x:%2x:%2x   \n", ap_records[i].ssid, ap_records[i].primary, ap_records[i].rssi, *ap_records[i].bssid, *(ap_records[i].bssid + 1), *(ap_records[i].bssid + 2), *(ap_records[i].bssid + 3), *(ap_records[i].bssid + 4), *(ap_records[i].bssid + 5));

		char dst[75];
		memcpy(dst, ap_records[i].ssid, 33);
		char test[33] = "Jeremy";
		// printf(" \n %s %s \n "test2,test);
		if (strcmp(dst, test) == NULL)
		{
			printf("\n\n\nyoupi\n\n\n");
		}
		//  printf("----------------------------------------------------------------\n");
	}
	printf("\n\n ICI SSID ts_credentials\n ######### %s #######", get_svrdata()->credentials.SSID);
}