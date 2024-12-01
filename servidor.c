#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080            // Puerto por defecto
#define BUFFER_SIZE 1024     // data buffer size
#define MAX_CLIENTS 10       // Max clients

int keep_running = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //crear el semaforo

// Función para manejar la señal SIGINT y cerrar el servidor de forma segura
void handle_sigint(int sig) {
    keep_running = 0;
}

// Función para enviar alertas (aquí puedes personalizar el método de envío)
void enviar_alerta(const char *mensaje) {
     // Escapar el mensaje para usarlo en la línea de comandos
    char comando[512];
    snprintf(comando, sizeof(comando), "python enviarAlerta.py \"%s\"", mensaje);

    // Ejecutar el comando
    int resultado = system(comando);
    if (resultado == -1) {
        perror("[ERROR]: No se pudo ejecutar el script de Python");
    }
}

// Función para procesar los datos recibidos del agente
void procesar_datos(const char *datos) {
    pthread_mutex_lock(&mutex);

    printf("[INFO]: Procesando datos: %s\n", datos);

    char servicio[256];
    int alertas = 0, errores = 0;

    if (sscanf(datos, "{ \"servicio\": \"%255[^\"]\", \"alertas\": %d, \"errores\": %d }",
               servicio, &alertas, &errores) == 3) {
        printf("[INFO]: Servicio: %s, Alertas: %d, Errores: %d\n", servicio, alertas, errores);


        if (errores > 10) {
            char mensaje[512];
            snprintf(mensaje, sizeof(mensaje),
                     "Servicio en error: %s. Demasiados errores (%d).", servicio, errores);
            enviar_alerta(mensaje);
        }
    } else {
        fprintf(stderr, "[ERROR]: Formato de datos no válido.\n");
    }

    pthread_mutex_unlock(&mutex);
}

// Función manejadora para cada cliente
void *manejar_cliente(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos;

    while ((bytes_leidos = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_leidos] = '\0'; // Asegurar terminación de la cadena
        printf("[INFO]: Datos recibidos: %s\n", buffer);

        procesar_datos(buffer);
    }

    if (bytes_leidos == 0) {
        printf("[INFO]: Cliente desconectado.\n");
    } else if (bytes_leidos < 0) {
        perror("[ERROR]: Error al recibir datos del cliente");
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // CTRTL +C para detener el servidor
    signal(SIGINT, handle_sigint);

    // Crear socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Enlazar socket al puerto
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: Error al enlazar el socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_sock, MAX_CLIENTS) == -1) {
        perror("[ERROR]: Error al escuchar en el puerto");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[INFO]: Servidor escuchando en el puerto %d...\n", PORT);

    while (keep_running) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            if (keep_running) {
                perror("[ERROR]: Error al aceptar conexión");
            }
            continue;
        }

        printf("[INFO]: Cliente conectado desde %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Crear un hilo para manejar al cliente
        pthread_t cliente_thread;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;

        if (pthread_create(&cliente_thread, NULL, manejar_cliente, client_sock_ptr) != 0) {
            perror("[ERROR]: Error al crear el hilo para el cliente");
            close(client_sock);
            free(client_sock_ptr);
        } else {
            pthread_detach(cliente_thread); // Liberar recursos al finalizar el hilo
        }
    }

    printf("[INFO]: Deteniendo servidor...\n");
    close(server_sock);
    return 0;
}
