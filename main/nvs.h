#ifndef __NVS_H__
#define __NVS_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>


void fct_write_flash(char* p_key, uint8_t* p_out_value, size_t p_length);
void fct_read_flash(char* p_key, uint8_t* p_out_value, size_t* p_length);

#endif