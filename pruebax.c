#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int load_env(const char *filename, char *account_sid, size_t sid_size, char *auth_token, size_t token_size) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo .env");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "TWILIO_ACCOUNT_SID=", 19) == 0) {
            strncpy(account_sid, line + 19, sid_size);
            account_sid[strcspn(account_sid, "\n")] = 0; // Eliminar salto de línea
        } else if (strncmp(line, "TWILIO_AUTH_TOKEN=", 18) == 0) {
            strncpy(auth_token, line + 18, token_size);
            auth_token[strcspn(auth_token, "\n")] = 0; // Eliminar salto de línea
        }
    }

    fclose(file);
    return 0;
}

void send_whatsapp_message(const char *account_sid, const char *auth_token, const char *to, const char *from, const char *body) {
    CURL *curl;
    CURLcode res;

    char url[512];
    snprintf(url, sizeof(url),
             "https://api.twilio.com/2010-04-01/Accounts/%s/Messages.json",
             account_sid);

    curl = curl_easy_init();
    if (curl) {
        char credentials[256];
        snprintf(credentials, sizeof(credentials), "%s:%s", account_sid, auth_token);

        char postfields[512];
        snprintf(postfields, sizeof(postfields),
                 "To=whatsapp:+%s&From=whatsapp:+%s&Body=%s",
                 to, from, body);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Error en curl: %s\n", curl_easy_strerror(res));
        } else {
            printf("Mensaje enviado con éxito\n");
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "No se pudo inicializar curl\n");
    }
}

int main() {
    char account_sid[128];
    char auth_token[128];

    if (load_env(".env", account_sid, sizeof(account_sid), auth_token, sizeof(auth_token)) != 0) {
        return 1;
    }

    const char *to = "593986849600";
    const char *from = "14155238886";
    const char *body = "Hola, este es un mensaje de prueba desde C";

    send_whatsapp_message(account_sid, auth_token, to, from, body);

    return 0;
}
