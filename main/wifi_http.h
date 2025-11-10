#ifndef _WIFI_HTTP_H_
#define _WIFI_HTTP_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define WIFI_SSID      "Nice 2.4G"
#define WIFI_PASSWORD  "bolinho12"
#define HTTP_SERVER_URL "http://192.168.1.13/api/data"

/**
 * @brief Tenta conectar ao Wi-Fi com um timeout.
 * 
 * @param timeout_ms Tempo máximo para tentar a conexão em milissegundos.
 * @return true se a conexão for bem-sucedida, false caso contrário.
 */
bool wifi_connect(int timeout_ms);

/**
 * @brief Envia os dados de pressão e temperatura via HTTP POST.
 * 
 * @param pressure Valor da pressão.
 * @param temperature Valor da temperatura.
 * @return true se o envio for bem-sucedido, false caso contrário.
 */
bool http_post_data(float pressure, float temperature);

#ifdef __cplusplus
}
#endif

#endif
