#include "nvs.h"

void fct_write_flash(char *p_key, uint8_t *p_out_value, size_t p_length)
{
   // Initialize NVS
   esp_err_t err = nvs_flash_init();
   if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
   {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
   }
   ESP_ERROR_CHECK(err);

   // Open
   // printf("\n");
   // printf("Opening Non-Volatile Storage (NVS) handle... ");
   nvs_handle_t my_handle;
   err = nvs_open("storage", NVS_READWRITE, &my_handle);
   if (err != ESP_OK)
   {
      printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
   }
   else
   {
      // printf("nvs_open Done\n");
      err = nvs_set_blob(my_handle, p_key, p_out_value, p_length);
   }
   nvs_close(my_handle);
}



void fct_read_flash(char* p_key, uint8_t* p_out_value, size_t* p_length)
{
   // Initialize NVS
   esp_err_t err = nvs_flash_init();
   if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
   {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
   }
   ESP_ERROR_CHECK(err);

   // Open
   // printf("\n");
   // printf("Opening Non-Volatile Storage (NVS) handle... ");
   nvs_handle_t my_handle;
   err = nvs_open("storage", NVS_READWRITE, &my_handle);
   if (err != ESP_OK)
   {
      printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
   }
   else
   {
      // printf("nvs_open Done\n");

      // Read
      err = nvs_get_blob(my_handle, p_key, p_out_value, p_length);
   }
   nvs_close(my_handle);
}