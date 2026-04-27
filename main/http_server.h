#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o servidor HTTP.
 * 
 * @return httpd_handle_t Handle do servidor, ou NULL em caso de falha.
 */
httpd_handle_t start_webserver(void);

/**
 * @brief Para o servidor HTTP.
 * 
 * @param server_handle Handle do servidor.
 */
void stop_webserver(httpd_handle_t server_handle);

#ifdef __cplusplus
}
#endif

#endif
