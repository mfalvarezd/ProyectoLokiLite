#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// Función para leer las credenciales del archivo .env
void load_env(const char *filename, char *account_sid, char *auth_token) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("No se pudo abrir el archivo .env");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (key && value) {
            if (strcmp(key, "ACCOUNT_SID") == 0) {
                strncpy(account_sid, value, 128);
                account_sid[127] = '\0'; // Asegurar que esté terminado en null
            } else if (strcmp(key, "AUTH_TOKEN") == 0) {
                strncpy(auth_token, value, 128);
                auth_token[127] = '\0'; // Asegurar que esté terminado en null
            }
        }
    }

    fclose(file);
}

void send_whatsapp_message(const char *account_sid, const char *auth_token) {
    CURL *curl;
    CURLcode res;

    // Valores quemados
    const char *to = "+593986849600";   // Número de destino
    const char *from = "+14155238886"; // Número de Twilio
    const char *body = "Hola, este es un mensaje de prueba desde C";

    // Construir la URL de la API de Twilio
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json",
             account_sid);

    // Inicializar libcurl
    curl = curl_easy_init();
    if (curl) {
        // Configurar autenticación básica
        char credentials[256];
        snprintf(credentials, sizeof(credentials), "%s:%s", account_sid, auth_token);

        // Configurar los datos del POST
        char postfields[512];
        snprintf(postfields, sizeof(postfields),
                 "To=whatsapp:%s&From=whatsapp:%s&Body=%s",
                 to, from, body);

        // Configurar opciones de curl
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC); // Autenticación básica

        // Realizar la solicitud
        res = curl_easy_perform(curl);

        // Manejar la respuesta
        if (res != CURLE_OK) {
            fprintf(stderr, "Error en curl: %s\n", curl_easy_strerror(res));
        } else {
            printf("Mensaje enviado con éxito\n");
        }

        // Limpiar
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "No se pudo inicializar curl\n");
    }
}

int main() {
    // Buffers para cargar las credenciales del .env
    char account_sid[128] = {0};
    char auth_token[128] = {0};

    // Cargar credenciales del archivo .env
    load_env(".env", account_sid, auth_token);

    // Llamar a la función para enviar el mensaje
    send_whatsapp_message(account_sid, auth_token);

    return 0;
}
