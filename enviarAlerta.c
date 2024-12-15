#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

// Función para eliminar saltos de línea y espacios extra
void trim_newline(char *str) {
    char *pos;
    if ((pos = strchr(str, '\n')) != NULL) {
        *pos = '\0';
    }
    if ((pos = strchr(str, '\r')) != NULL) {
        *pos = '\0';
    }
}

// Función para obtener el valor de una clave específica en el archivo .env
int get_env_value(const char *filename, const char *key, char *value, size_t value_size) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("[ERROR]: No se pudo abrir el archivo .env");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);

        char *equal_sign = strchr(line, '=');
        if (equal_sign) {
            *equal_sign = '\0'; // Divide en clave y valor
            char *key_in_file = line;
            char *value_in_file = equal_sign + 1;

            if (strcmp(key, key_in_file) == 0) {
                strncpy(value, value_in_file, value_size - 1);
                value[value_size - 1] = '\0';
                fclose(file);
                return 0; // Valor encontrado
            }
        }
    }

    fclose(file);
    fprintf(stderr, "[ERROR]: Clave %s no encontrada en el archivo .env\n", key);
    return -1; // Clave no encontrada
}

int main(int argc, char *argv[]) {
    const char *env_file = ".env"; // Nombre del archivo .env

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <mensaje>\n", argv[0]);
        return 1;
    }

    CURL *curl;
    CURLcode res;

    char account_sid[MAX_LINE_LENGTH];
    char auth_token[MAX_LINE_LENGTH];
    char to_number[MAX_LINE_LENGTH];
    char from_number[MAX_LINE_LENGTH];

    // Leer valores del archivo .env
    if (get_env_value(env_file, "TWILIO_ACCOUNT_SID", account_sid, sizeof(account_sid)) != 0) {
        return 1;
    }

    if (get_env_value(env_file, "TWILIO_AUTH_TOKEN", auth_token, sizeof(auth_token)) != 0) {
        return 1;
    }

    if (get_env_value(env_file, "TONUMBER", to_number, sizeof(to_number)) != 0) {
        return 1;
    }

    if (get_env_value(env_file, "FROMNUMBER", from_number, sizeof(from_number)) != 0) {
        return 1;
    }

    const char *url = "https://api.twilio.com/2010-04-01/Accounts/";
    char full_url[256];
    snprintf(full_url, sizeof(full_url), "%s%s/Messages.json", url, account_sid);

    char post_fields[512];
    snprintf(post_fields, sizeof(post_fields), "To=whatsapp:%s&From=whatsapp:%s&Body=%s", to_number, from_number, argv[1]);

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        curl_easy_setopt(curl, CURLOPT_USERNAME, account_sid);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, auth_token);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Error en curl: %s\n", curl_easy_strerror(res));
        } else {
            printf("Mensaje enviado con éxito\n");
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "[ERROR]: No se pudo inicializar curl\n");
    }

    return 0;
}
