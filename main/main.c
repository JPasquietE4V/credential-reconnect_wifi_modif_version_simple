#include "apsta.h"
#include "websocket.h"
#include "nvs.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>

#include "apsta_shared.h"

#include <driver/gpio.h>

#include <esp_task_wdt.h>

extern const char *TAG_APSTA;
extern const char *TAGWS;

#define GPIN GPIO_NUM_12

static QueueHandle_t msg_queue;
void xtask_wifi_scanning(void *args)
{
	while (1)
	{
		//printf("entrée dans la task \n");
		if (get_svrdata()->wifi_initialized == true)
		{
			printf("\n\n\n ############## wifi is initalised, so I can scann ######################### \n\n");
		//	scann_wifi_around();
		}
	//	printf("sortie de la task \n");
			vTaskDelay(35000 / portTICK_PERIOD_MS);
		//	esp_wifi_disconnect();
		//	esp_wifi_stop();
			vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void xtask_blink(void *args)
{
	static int cnt = 0;
	unsigned int f = 10;
	while (1)
	{
		if (xQueueReceive(msg_queue, (void *)&f, 0) == pdTRUE)
		{

			// sprintf(dbg, "Queue received  value = %d\n", f);
			// ESP_LOGI("Blink", "Queue \b");
			// printf(dbg);
		}
		gpio_set_level(GPIN, cnt % 2);
		cnt++;
		vTaskDelay(f / portTICK_PERIOD_MS);
	}
}

/*
	Tâche à l'attente de changement des credentials SSID, PASS
*/
void xtask_evnt(void *args)
{
	const EventBits_t xBitsToWaitFor = (EventBits_t)CRED_COMPLETE;
	EventBits_t xEventGroupValue;
	static CRED_DATA_T recvbits = CRED_NONE;

	while (1)
	{
		// On recupere l'état des flags si les credentials ont été modifiés

		xEventGroupValue = xEventGroupWaitBits(
			get_svrdata()->sync_event_group, // stockage des flags
			xBitsToWaitFor,					 // bits à attente
			pdFALSE,						 // Clear on Exit
			pdTRUE,							 // wait for all bits
			0);
		xEventGroupClearBits(
			get_svrdata()->sync_event_group, /* The event group being updated. */
			(EventBits_t)CRED_COMPLETE);	 /* The bits being cleared. */

		if ((xEventGroupValue & (EventBits_t)CRED_SSID) != 0)
		{
			recvbits |= CRED_SSID;
			ESP_LOGI("event", "got ssid");
		}
		if ((xEventGroupValue & (EventBits_t)CRED_PASS) != 0)
		{
			recvbits |= CRED_PASS;
			ESP_LOGI("event", "got pass");
		}
		if ((recvbits & CRED_COMPLETE) == CRED_COMPLETE)
		{
			ESP_LOGI("Event handler", "Credentials complete");
			vTaskDelay(50 / portTICK_PERIOD_MS);
			ESP_LOGW("Event handler", "New credentials");
			printf("SSID: %s, PASS: %s\n", get_ssid(), get_pwd());
			ESP_LOGI("Event handler", "Copying credentials to sta_config.sta");
			strcpy((char *)(get_svrdata()->sta_config.sta.ssid), get_ssid());
			strcpy((char *)(get_svrdata()->sta_config.sta.password), get_pwd());

			ESP_LOGI("Event handler", "Stopping wifi");
			esp_wifi_disconnect();
			esp_wifi_stop();
			// Effacer les bits
			xEventGroupClearBits(
				get_svrdata()->sync_event_group,
				(EventBits_t)CRED_COMPLETE);
			recvbits &= (~CRED_COMPLETE);

			wifi_apsta(CONFIG_STA_CONNECT_TIMEOUT * 1000);

			ESP_LOGI("Event handler", "Restart DONE! \n");
		}
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	static httpd_handle_t server = NULL;
	msg_queue = xQueueCreate(2, sizeof(int));

	// zero-initialize the config structure.
	gpio_config_t io_conf = {};
	// disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	// bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = (1ULL << GPIN);
	// disable pull-down mode
	io_conf.pull_down_en = 0;
	// disable pull-up mode
	io_conf.pull_up_en = 0;
	// configure GPIO with the given settings
	esp_err_t error = gpio_config(&io_conf);

	static unsigned int frequence = 500;

	if (error != ESP_OK)
	{
		printf("error configuring outputs \n");
	}
	else
	{
		printf("GPIO pins OK\n");
		gpio_set_level(GPIN, 0);
		xTaskCreate(xtask_blink, "gpio_task_blink", 2048, (void *)&frequence, 2, NULL);
		xTaskCreate(xtask_wifi_scanning, "wifi_task_scanning", 4096, NULL, 0, NULL);
	}

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
	/*******/

	apsta_init();

	get_svrdata()->sync_event_group = xEventGroupCreate();
	xTaskCreate(xtask_evnt, "tsk_event", 2048, NULL, 2, NULL);

	setup_httpws(get_svrdata());

	initialise_wifi();
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
	}

	/**********************************************/
	ESP_LOGW(TAG_APSTA, "Start APSTA Mode");

	wifi_apsta(CONFIG_STA_CONNECT_TIMEOUT * 1000);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));

	/* Debut du serveur pour la première fois */
	server = start_webserver();

	static int cnt = 0;
	while (1)
	{

		vTaskDelay(10 / portTICK_PERIOD_MS);
		cnt++;
		if ((cnt % 100) == 0)
		{
			if (frequence == 25)
			{
				frequence = 100;
			}
			else
			{
				frequence = 25;
			}
			xQueueSend(msg_queue, &frequence, portMAX_DELAY);

			cnt = 0;
		}
	}
}