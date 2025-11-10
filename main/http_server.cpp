extern "C" {
    #include "esp_err.h"
    #include "esp_log.h"
    #include "esp_http_server.h"
}
#include <cstring>
#include <cstdio>

static const char *TAG = "HTTP_SERVER";

/* Manipulador para arquivos estáticos */
esp_err_t static_file_handler(httpd_req_t *req)
{
    char filename[256];
    const char *uri = httpd_req_get_uri(req);

    // Cópia segura para o buffer (sem risco de overflow)
    strncpy(filename, uri, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    ESP_LOGI(TAG, "Servindo arquivo: %s", filename);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "Arquivo recebido com sucesso!", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Inicializa o servidor HTTP */
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t uri_static = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = static_file_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &uri_static);

        ESP_LOGI(TAG, "Servidor HTTP iniciado na porta %d", config.server_port);
        return server;
    }

    ESP_LOGE(TAG, "Falha ao iniciar o servidor HTTP");
    return NULL;
}


/* Função para parar o servidor */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
        ESP_LOGI(TAG, "Servidor HTTP parado");
    }
}
