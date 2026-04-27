extern "C" {
    #include "esp_err.h"
    #include "esp_log.h"
    #include "esp_http_server.h"
    #include "esp_vfs.h" // Necessário para operações de sistema de arquivos
    #include "esp_spiffs.h" // Necessário para SPIFFS
}
#include <cstring>
#include <cstdio>
#include <sys/param.h> 
#include <string.h>
#include "cSMP3011.h"


extern cSMP3011 SMP3011;

static const char *TAG = "HTTP_SERVER";

// Função para manipular a requisição GET para /api/data
esp_err_t api_data_handler(httpd_req_t *req)
{
    
    float pressure_bar = SMP3011.getPressure() * 0.01; 
    float temperature_celsius = SMP3011.getTemperature(); 
    float pressure_psi = SMP3011.getPressure() * 0.145038;

    //Criar a string JSON
    char json_response[128];
    
    
    int len = snprintf(json_response, sizeof(json_response),
                       "{\"pressure_bar\": %.2f, \"pressure_psi\": %.2f, \"temperature\": %.1f}",
                       pressure_bar, pressure_psi, temperature_celsius);

    //tipo de conteúdo como JSON
    httpd_resp_set_type(req, "application/json");

    // 5. Enviar a resposta JSON
    if (len > 0 && len < sizeof(json_response)) {
        httpd_resp_send(req, json_response, len);
    } else {
        httpd_resp_send_500(req);
    }

    return ESP_OK;
}
// O ponto de montagem do SPIFFS 
#define BASE_PATH "/spiffs" 

// Função auxiliar para obter o tipo de conteúdo
static const char* get_content_type_from_uri(const char *uri)
{
    if (strstr(uri, ".css")) {
        return "text/css";
    } else if (strstr(uri, ".js")) {
        return "application/javascript";
    } else if (strstr(uri, ".png")) {
        return "image/png";
    } else if (strstr(uri, ".jpg")) {
        return "image/jpeg";
    } else if (strstr(uri, ".ico")) {
        return "image/x-icon";
    }
    return "text/html";
}

extern "C" {
    /* Manipulador para arquivos estáticos */
    esp_err_t static_file_handler(httpd_req_t *req  )
    {
        char filepath[256];
        const char *uri = req->uri;

        
        //Construir o caminho completo do arquivo no SPIFFS
        
        if (strcmp(uri, "/") == 0) {   
            snprintf(filepath, sizeof(filepath), "%s/index.html", BASE_PATH);
        } else {

            size_t base_len = strlen(BASE_PATH);
            strncpy(filepath, BASE_PATH, sizeof(filepath));
            filepath[sizeof(filepath) - 1] = '\0'; 

            size_t remaining_len = sizeof(filepath) - base_len - 1; 
            
            if (remaining_len > 0) {
                strncat(filepath, uri, remaining_len);
                filepath[sizeof(filepath) - 1] = '\0'; 
            }
        }

        // ABRIR O ARQUIVO E TRATAR ERRO 404
        FILE *file = fopen(filepath, "r");
        if (!file) {
            ESP_LOGE(TAG, "Falha ao abrir arquivo: %s", filepath);
            httpd_resp_send_404(req );
            return ESP_FAIL;
        }

        //Obter o tamanho do arquivo
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Definir o tipo de conteúdo
        const char *content_type = get_content_type_from_uri(filepath);
        httpd_resp_set_type(req, content_type  );

        // Enviar o conteúdo do arquivo
        char *chunk = (char*)malloc(file_size);
        if (chunk) {
            size_t read_bytes = fread(chunk, 1, file_size, file);
            httpd_resp_send(req, chunk, read_bytes  );
            free(chunk);
        } else {
            ESP_LOGE(TAG, "Falha ao alocar memória para o arquivo");
            httpd_resp_send_500(req  );
        }

        fclose(file);
        ESP_LOGI(TAG, "Servindo arquivo: %s (Tamanho: %ld, Tipo: %s)", filepath, file_size, content_type);
        return ESP_OK;
    }

    /* Inicializa o servidor HTTP */
    httpd_handle_t start_webserver(void )
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG( );
        config.server_port = 80;

        httpd_handle_t server = NULL;
        if (httpd_start(&server, &config ) == ESP_OK)
        {
            // Manipulador para o endpoint /api/data
            httpd_uri_t uri_api_data = {
                .uri      = "/api/data",
                .method   = HTTP_GET,
                .handler  = api_data_handler,
                .user_ctx = NULL
            };
            httpd_register_uri_handler(server, &uri_api_data);

            // Manipulador curinga para servir arquivos estáticos
            // O URI curinga deve ser o último a ser registrado
            httpd_uri_t uri_static = {
                .uri = "/index.htm",
                .method = HTTP_GET,
                .handler = static_file_handler,
                .user_ctx = NULL
            };
            httpd_register_uri_handler(server, &uri_static );

            ESP_LOGI(TAG, "Servidor HTTP iniciado na porta %d", config.server_port);

            
            ESP_LOGE(TAG, "Listando arquivos no diretório: %s", BASE_PATH);

                    DIR* dir = opendir(BASE_PATH);
                    if (dir) {
                        struct dirent* entry;
                        while ((entry = readdir(dir)) != NULL) {
                            if (entry->d_type == DT_DIR) {
                                ESP_LOGE(TAG, "  DIR: %s", entry->d_name);
                            } else {
                                ESP_LOGE(TAG, "  FILE: %s", entry->d_name);
                           }
                        }
                    }
                    
            return server;
        }

        ESP_LOGE(TAG, "Falha ao iniciar o servidor HTTP");
        return NULL;
    }

    /* Função para parar o servidor */
    void stop_webserver(httpd_handle_t server )
    {
        if (server) {
            httpd_stop(server );
            ESP_LOGI(TAG, "Servidor HTTP parado");
        }
    }
}
