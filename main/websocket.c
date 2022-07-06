/* WebSocket Echo Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicablinitialise_wifi()e law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "websocket.h"
#include "apsta.h"
#include "simplecrc.h"
#include "nvs.h"

#define CREDENTIALS_LEN 65
#define STREND (CREDENTIALS_LEN - 1)
/* A simple example that demonstrates using websocket echo server
 */
extern const uint8_t html_file_array[] asm("_binary_upload_html_start");
const char *TAGWS = "ws_echo_server"; // attribut privé, visible uniquement dans le fichier

// httpd_uri_t
/*
structure which has members including uri name, method type (eg. HTTPD_GET/HTTPD_POST/HTTPD_PUT etc.),
function pointer of type esp_err_t *handler (httpd_req_t *req) and user_ctx pointer to user context data.
*/

void setup_httpws(psapstadata_t psvdata)
{
    psvdata->credential.uri = "/";
    psvdata->credential.method = HTTP_GET;
    psvdata->credential.handler = credential_get_handler;
    psvdata->credential.user_ctx = (uint8_t *)html_file_array; // CAST, pas de warnings (const)

    psvdata->ws.uri = "/ws";
    psvdata->ws.method = HTTP_GET;
    psvdata->ws.handler = ws_handler;
    psvdata->ws.user_ctx = NULL;
    psvdata->ws.is_websocket = true;
}

char *get_ssid(int p)
{
    return get_svrdata()->credentials[p].SSID; //.SSID[0] // get_svrdata()->SSID;
}

char *get_pwd(int p)
{
    return get_svrdata()->credentials[p].PASS; //.PASS[0] // get_svrdata()->PWD;
}

void set_ssid(char *var_ssid, int par_int_n)
{
    strcpy(get_svrdata()->credentials[par_int_n].SSID, var_ssid); //.SSID[0] // strcpy(get_svrdata()->SSID,var_ssid);
}

void set_pwd(char *var_pwd, int par_int_n)
{
    strcpy(get_svrdata()->credentials[par_int_n].PASS, var_pwd); //.PASS[0] // strcpy(get_svrdata()->PWD,var_pwd);
}

/*
 * async send function, which we put into the httpd work queue
 */
void ws_async_send(void *arg)
{
    static const char *data = "Async data";
    /*struct*/ async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd; // -> pour acceder au membre d'une structure
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    /*struct*/ async_resp_arg *resp_arg = malloc(sizeof(/*struct*/ async_resp_arg));
    resp_arg->hd = req->handle;

    // httpd_req_to_sockfd
    /*
    Get the Socket Descriptor from the HTTP request.

    This API will return the socket descriptor of the session for which URI handler was executed on reception of HTTP request.

    Parameters
        r – [in] The request whose socket descriptor should be found

    Returns
        Socket descriptor : The socket descriptor for this request
        -1 : Invalid/NULL request pointer
    */
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}

esp_err_t credential_get_handler(httpd_req_t *req) // ledOFF_get_handler --> hello_get_handler
{
    esp_err_t error;
    // ESP_LOGI(TAGWS, "LED Turned OFF");
    const char *response = (const char *)req->user_ctx;
    error = httpd_resp_send(req, response, strlen(response));
    if (error != ESP_OK)
    {
        ESP_LOGI(TAGWS, "Error %d while sending Response", error);
    }
    else
        ESP_LOGI(TAGWS, "Response sent Successfully");
    return error;
}

esp_err_t ws_handler(httpd_req_t *req)
{
    // const size_t size_un_wifi = sizeof(ts_credentials) / sizeof(uint8_t);
    const size_t size_tous_les_wifis = NB_WIFI_MAX * sizeof(ts_credentials) / sizeof(uint8_t);

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAGWS, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;

    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAGWS, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAGWS, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len)
    {
        uint8_t size_ws_pkt = ws_pkt.len;
        char buf_ssid[size_ws_pkt];
        char buf_pass[size_ws_pkt];

        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAGWS, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;

        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAGWS, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAGWS, "Got packet with message: %s", ws_pkt.payload);

        if (strstr((char *)ws_pkt.payload, "SSID"))
        {
            for (int i = 0; i < size_ws_pkt; i++)
            {
                buf_ssid[i] = *(ws_pkt.payload + 7 + i);
            }

            buf_ssid[size_ws_pkt] = '\0';

            strcpy(get_svrdata()->credentials_recus.SSID, buf_ssid);
            printf("2\n");
        }

        if (strstr((char *)ws_pkt.payload, "PASS"))
        {
            for (int i = 0; i < size_ws_pkt; i++)
            {
                buf_pass[i] = *(ws_pkt.payload + 7 + i);
            }

            buf_pass[size_ws_pkt] = '\0';

            strcpy(get_svrdata()->credentials_recus.PASS, buf_pass);

            //*** Choix de l'emplacement du nouveau réseau ***
            // on pourrait directement mettre l'égalité entre credential[i] et credential_reçu

                for (int i = 0; i < NB_WIFI_MAX; i++)
                {
                    if (get_svrdata()->credentials[i].CRC32 == 0xb84614a0)
                    {
                        strcpy(get_svrdata()->credentials[i].SSID, get_svrdata()->credentials_recus.SSID);
                        strcpy(get_svrdata()->credentials[i].PASS, get_svrdata()->credentials_recus.PASS);
                        uint32_t crc = 0;
                        crc32(get_svrdata()->credentials[i].SSID, get_svrdata()->credentials[i].PASS, &crc);
                        get_svrdata()->credentials[i].CRC32 = crc;
                         printf("W SSID : %s\n PASS : %s\n CRC32 : %x\n\n", get_svrdata()->credentials[i].SSID,
                         get_svrdata()->credentials[i].PASS, get_svrdata()->credentials[i].CRC32);

                        break;
                    }
                    else
                    {
                        if (i == (NB_WIFI_MAX - 1))
                        {
                            printf("Desole, la limite de réseaux wifi enregistrés a été atteinte \n");
                        }
                    }
                }

            // remise a 0
            /*   for (int i = 0; i < NB_WIFI_MAX; i++)
                {
                    strcpy(get_svrdata()->credentials[i].SSID, get_svrdata()->credentials_recus.SSID);
                    strcpy(get_svrdata()->credentials[i].PASS, get_svrdata()->credentials_recus.PASS);
                    uint32_t crc = 0;
                    crc32(get_svrdata()->credentials[i].SSID, get_svrdata()->credentials[i].PASS, &crc);
                    get_svrdata()->credentials[i].CRC32 = crc;

                       printf("W SSID : %s\n PASS : %s\n CRC32 : %x\n\n", get_svrdata()->credentials[i].SSID,
                    get_svrdata()->credentials[i].PASS, get_svrdata()->credentials[i].CRC32);
                }*/

            fct_write_flash("credentials", (uint8_t *)&get_svrdata()->credentials, size_tous_les_wifis);
            printf("/////////////////////////////////////////////\n");
            for (int i = 0; i < NB_WIFI_MAX; i++)
            {
                printf("R SSID : %s\n PASS : %s\n CRC32 : %x\n\n", get_svrdata()->credentials[i].SSID,
                       get_svrdata()->credentials[i].PASS, get_svrdata()->credentials[i].CRC32);
            }
            printf("/////////////////////////////////////////////\n");
        }

        if (strstr((char *)ws_pkt.payload, "SUPP"))
        {
            for (int i = 0; i < NB_WIFI_MAX; i++)
            {
                strcpy(get_svrdata()->credentials[i].SSID, "0");
                strcpy(get_svrdata()->credentials[i].PASS, "0");
                uint32_t crc = 0;
                crc32(get_svrdata()->credentials[i].SSID, get_svrdata()->credentials[i].PASS, &crc);
                get_svrdata()->credentials[i].CRC32 = crc;

                printf("W SSID : %s\n PASS : %s\n CRC32 : %x\n\n", get_svrdata()->credentials[i].SSID,
                       get_svrdata()->credentials[i].PASS, get_svrdata()->credentials[i].CRC32);
            }

            fct_write_flash("credentials", (uint8_t *)&get_svrdata()->credentials, size_tous_les_wifis);
            printf("----------------------------------------------\n");
            for (int i = 0; i < NB_WIFI_MAX; i++)
            {
                printf("R SSID : %s\n PASS : %s\n CRC32 : %x\n\n", get_svrdata()->credentials[i].SSID,
                       get_svrdata()->credentials[i].PASS, get_svrdata()->credentials[i].CRC32);
            }
            printf("----------------------------------------------\n");
        }
        //        uint8_t taille = ws_pkt.len;
        //        char sub_ssid[taille];
        //        char sub_pwd[taille];
        //
        //        /**********************************/
        //        /********** NVS WRITE *************/
        //        /**********************************/
        //        // Initialize NVS
        //        esp_err_t err3 = nvs_flash_init();
        //        if (err3 == ESP_ERR_NVS_NO_FREE_PAGES || err3 == ESP_ERR_NVS_NEW_VERSION_FOUND)
        //        {
        //            // NVS partition was truncated and needs to be erased
        //            // Retry nvs_flash_init
        //            ESP_ERROR_CHECK(nvs_flash_erase());
        //            err3 = nvs_flash_init();
        //        }
        //        ESP_ERROR_CHECK(err3);
        //
        //        // Open
        //        printf("\n");
        //        printf("Opening Non-Volatile Storage (NVS) handle... ");
        //        nvs_handle_t my_handle;
        //        err3 = nvs_open("storage", NVS_READWRITE, &my_handle);
        //        if (err3 != ESP_OK)
        //        {
        //            printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err3));
        //        }
        //        else
        //        {
        //
        //            printf("nvs_open Done\n");
        //            // Read
        //            // REMARQUE 3 pourquoi déclarer cette structure de données si on l'as déjà sur svr_data.credentials
        //            // meme remarque que sur main.c ligne 182
        //            // ts_credentials my_struct[NB_WIFI_MAX]; // <--- NON, PAS BESOIN DE CETTE REDONDANCE !
        //
        //            size_t size_un_wifi = sizeof(ts_credentials) / sizeof(uint8_t);
        //
        //            for (int i = 0; i < NB_WIFI_MAX; i++)
        //            {
        //                get_svrdata()->credentials[i].SSID[STREND] = 0;
        //                get_svrdata()->credentials[i].PASS[STREND] = 0;
        //                // my_struct[i].SSID[STREND] = 0; // au cas ou
        //                // my_struct[i].PASS[STREND] = 0; // au cas ou
        //            }
        //
        //            /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        //            // REMARQUE 1 : je ne comprends pas le role du buf ici, jamais vraiment utilisé....
        //            buf = calloc(1, ws_pkt.len + 1);
        //            if (buf == NULL)
        //            {
        //                ESP_LOGE(TAGWS, "Failed to calloc memory for buf");
        //                return ESP_ERR_NO_MEM;
        //            }
        //            ws_pkt.payload = buf;
        //            /* Set max_len = ws_pkt.len to get the frame payload */
        //            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        //            if (ret != ESP_OK)
        //            {
        //                ESP_LOGE(TAGWS, "httpd_ws_recv_frame failed with %d", ret);
        //                free(buf);
        //                return ret;
        //            }
        //            ESP_LOGI(TAGWS, "Got packet with message: %s", ws_pkt.payload);
        //
        //            if (strstr((char *)ws_pkt.payload, "SSID"))
        //            {
        //                for (int i = 0; i < taille; i++)
        //                {
        //                    sub_ssid[i] = *(ws_pkt.payload + 7 + i);
        //                }
        //
        //                sub_ssid[taille] = '\0';
        //                set_ssid(sub_ssid, 0); ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        //                for (int i = 0; i < NB_WIFI_MAX; i++)
        //                {
        //                    // strcpy(my_struct[i].SSID, sub_ssid);
        //                    strcpy(get_svrdata()->credentials[i].SSID, sub_ssid);
        //                }
        //                xEventGroupSetBits(get_svrdata()->sync_event_group, (EventBits_t)CRED_SSID);
        //                printf("\n\nSUB SSID = %s\n", sub_ssid);
        //                strcpy(get_svrdata()->credentials_recus.SSID, sub_ssid);
        //            }
        //
        //            // REMARQUE 2 pourquoi faire get_blob ici ????
        //
        //            if (strstr((char *)ws_pkt.payload, "PASS"))
        //            {
        //                // err3 = nvs_get_blob(my_handle, "ssid", (uint8_t *)&my_struct, // <<<--- ???
        //                //                     &size_un_wifi);
        //                err3 = nvs_get_blob(my_handle, "ssid", (uint8_t *)&get_svrdata()->credentials,
        //                                    &size_un_wifi);
        //
        //                for (int i = 0; i < NB_WIFI_MAX; i++)
        //                {
        //                    if (get_svrdata()->credentials[i].CRC32 == 0xf4dbdf21)
        //                    {
        //                        printf("####################### TRUE SSID #############################\n");
        //                        printf("i=%d \n ssid =%s \n crc=%d\n\n", i, get_svrdata()->credentials[i].SSID, get_svrdata()->credentials[i].CRC32);
        //                        // strcpy(sub_ssid, my_struct[i].SSID);
        //                        strcpy(sub_ssid, get_svrdata()->credentials[i].SSID);
        //                        break;
        //                    }
        //                    else
        //                    {
        //                        // printf("####################### FALSE SSID #############################\n");
        //                    }
        //                }
        //                for (int i = 0; i < taille; i++)
        //                {
        //                    sub_pwd[i] = *(ws_pkt.payload + 7 + i);
        //                }
        //                sub_pwd[taille] = '\0';
        //                set_pwd(sub_pwd, 0);
        //                strcpy(get_svrdata()->credentials_recus.PASS, sub_pwd);
        //
        //                printf("\n");
        //                for (int i = 0; i < NB_WIFI_MAX; i++)
        //                {
        //                    // strcpy(my_struct[i].PASS, sub_pwd);
        //                    if (get_svrdata()->credentials[i].CRC32 == (int)0xf4dbdf21)
        //                    {
        //                        printf("####################### TRUE PASS #############################\n");
        //                        printf("i=%d \n ssid =%s \n crc=%d\n\n", i, get_svrdata()->credentials[i].SSID, get_svrdata()->credentials[i].CRC32);
        //                        strcpy(get_svrdata()->credentials[i].PASS, sub_pwd);
        //                    }
        //                    else
        //                    {
        //                        // printf("####################### FALSE PASS #############################\n");
        //                    }
        //                }
        //                xEventGroupSetBits(get_svrdata()->sync_event_group, (EventBits_t)CRED_PASS);
        //                // printf("\n\nSUB SSID = %s\nSUB PASS = %s\n\n", sub_ssid, sub_pwd);
        //                printf("######################\n SSID = %s\n PASS = %s \n######################", get_svrdata()->credentials_recus.SSID, get_svrdata()->credentials_recus.PASS);
        //            }
        //
        //            /******** ECRITURE NVS***********/ //<--- REMARQUE 4 Mais tas deja faire un get_blob avant....
        //            //
        //            // ########### AJOUTS RECENTS ##############
        //            for (int i = 0; i < NB_WIFI_MAX; i++)
        //            {
        //                //                printf("****************\n");
        //                //                printf("Writing new_SSID = %s at position %d\n", get_svrdata()->credentials[i].SSID, i);
        //                //                printf("Writing new_PASS = %s at position %d\n", get_svrdata()->credentials[i].PASS, i);
        //                // strcpy(get_svrdata()->credentials[i].SSID, "0");
        //                // strcpy(get_svrdata()->credentials[i].PASS, "0");
        //                //           }
        //                printf("\n");
        //
        //                /*
        //                //#############################################a
        //                // err3 = nvs_set_blob(my_handle, "ssid", (uint8_t *)&my_struct,
        //                //                     size_tous_les_wifis);
        //
        //                // printf("\n\n");
        //                // for (int i = 0; i < NB_WIFI_MAX; i++)
        //                // {
        //                //     char buf[] = "";
        //                //     strcat ((char *)buf, get_svrdata()->credentials[i].SSID);
        //                //     strcat (buf, get_svrdata()->credentials[i].PASS);
        //                //     uint32_t crc = 0;
        //                //     int len = strlen(buf);
        //                //     printf("buf = %s \n", buf);
        //                //     crc32(buf, (len > 0) ? (len) : 0, &crc);
        //                //     get_svrdata()->credentials[i].CRC32 = crc;
        //                //     printf("Writing new_CRC = %x at position %d\n", get_svrdata()->credentials[i].CRC32, i);
        //                // }*/
        //
        //                /************* CRC ********************/
        //                //           for (int i = 0; i < NB_WIFI_MAX; i++)
        //                //           {
        //                uint32_t crc = 0;
        //                char concat_for_crc[] = "";
        //                strcat(concat_for_crc, get_svrdata()->credentials[i].SSID);
        //                strcat(concat_for_crc, get_svrdata()->credentials[i].PASS);
        //                int len = strlen(get_svrdata()->credentials[i].SSID) + strlen(get_svrdata()->credentials[i].PASS);
        //                crc32(concat_for_crc, (len > 0) ? (len) : 0, &crc);
        //                get_svrdata()->credentials[i].CRC32 = crc;
        //                //                printf(" ssid is %s\n pass is : %s \n size is %d\n buf is : %s\n CRC32 is : %x\n", get_svrdata()->credentials[i].SSID,
        //                //                       get_svrdata()->credentials[i].PASS,
        //                //                       len,
        //                //                       concat_for_crc,
        //                //                       get_svrdata()->credentials[i].CRC32);
        //                //                printf("****************\n\n");
        //            }
        //            //            printf("____________________________________\n");
        //            /**************************************/
        //
        //            // err3 = nvs_set_blob(my_handle, "ssid", (uint8_t *)&get_svrdata()->credentials,
        //            //                    size_tous_les_wifis);
        //
        //            strcpy(get_svrdata()->credentials[2].PASS, "toto");
        //            fct_write_flash("ssid", (uint8_t *)&get_svrdata()->credentials, size_tous_les_wifis);
        //
        //            vTaskDelay(100 / portTICK_PERIOD_MS);
        //            err3 = nvs_commit(my_handle);
        //            printf((err3 != ESP_OK) ? "Failed!\n" : "nvs_commit  Done\n");
        //            // Close
        //            vTaskDelay(100 / portTICK_PERIOD_MS);
        //            nvs_close(my_handle);
        //
        //            switch (err3)
        //            {
        //            case ESP_OK:
        //                printf("ESP_OK nvs_set_str \n");
        //                break;
        //            case ESP_ERR_NVS_NOT_FOUND:
        //                printf("The value is not initialized yet!\n");
        //                break;
        //            default:
        //                printf("Error (%s) reading!\n", esp_err_to_name(err3));
        //            }
        //        }
        //
        //        /********************************/
    }
    ESP_LOGI(TAGWS, "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
        strcmp((char *)ws_pkt.payload, "Trigger async") == 0)
    {
        free(buf);
        return trigger_async_send(req->handle, req);
    }

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAGWS, "httpd_ws_send_frame failed with %d", ret);
    }
    free(buf);
    return ret;
}

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAGWS, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Registering the ws handler
        ESP_LOGI(TAGWS, "Registering URI handlers");
        httpd_register_uri_handler(server, &(get_svrdata()->credential));
        httpd_register_uri_handler(server, &(get_svrdata()->ws));
        return server;
    }

    ESP_LOGI(TAGWS, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAGWS, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void connect_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAGWS, "Starting webserver");
        *server = start_webserver();
    }
}
