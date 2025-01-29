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
int server_sock;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Semáforo para sincronización

// Función para manejar la señal SIGINT y cerrar el servidor de forma segura
void handle_sigint(int sig) {
    printf("\n[INFO]: Señal SIGINT recibida. Cerrando servidor...\n");
    keep_running = 0;

    // Detener operaciones del socket para desbloquear `accept`
    shutdown(server_sock, SHUT_RDWR);
    close(server_sock);
}

// Función para enviar alertas
void enviar_alerta(const char *mensaje) {
    char comando[512];
    snprintf(comando, sizeof(comando), "./enviarAlerta \"%s\"", mensaje);

    int resultado = system(comando);
    if (resultado == -1) {
        perror("[ERROR]: No se pudo ejecutar el programa enviarAlerta");
    } else if (resultado != 0) {
        fprintf(stderr, "[ERROR]: El programa enviarAlerta devolvió un código de error %d\n", resultado);
    }
}

// Función para procesar los datos recibidos del agente
void procesar_datos(const char *datos) {
    pthread_mutex_lock(&mutex);

    printf("[INFO]: Procesando datos: %s\n", datos);

    float cpu_usage = 0.0;
    float memory_usage = 0.0;

    // Analizar los datos en formato JSON
    if (sscanf(datos, "{ \"cpu_usage\": %f, \"memory_usage\": %f }", &cpu_usage, &memory_usage) == 2) {
        printf("[INFO]: CPU Usage: %.2f%%, Memory Usage: %.2f%%\n", cpu_usage, memory_usage);

        // Verificar si cpu_usage o memory_usage superan el 80%
        if (cpu_usage > 80.0) {
            char mensaje[512];
            snprintf(mensaje, sizeof(mensaje), "Alerta: Uso de CPU ha superado el 80%%: %.2f%%", cpu_usage);
            enviar_alerta(mensaje);
        }
        if (memory_usage > 60.0) {
            char mensaje[512];
            snprintf(mensaje, sizeof(mensaje), "Alerta: Uso de Memoria ha superado el 60%%: %.2f%%", memory_usage);
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

    // Crear el pipe para este cliente
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("[ERROR]: No se pudo crear el pipe");
        close(client_sock);
        return NULL;
    }

    // Hilo para procesar los datos del pipe
    pthread_t pipe_thread;

    // Lógica para leer y procesar los datos de manera sincronizada
    while ((bytes_leidos = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_leidos] = '\0';  // Asegurar terminación de la cadena
        printf("[INFO]: Datos recibidos: %s\n", buffer);

        // Escribir en el pipe
        pthread_mutex_lock(&mutex);
        if (write(pipe_fd[1], buffer, bytes_leidos) == -1) {
            perror("[ERROR]: Error al escribir en el pipe");
        }
        pthread_mutex_unlock(&mutex);

        // Crear un hilo para procesar los datos si no está en ejecución
        if (pthread_create(&pipe_thread, NULL, (void *(*)(void *))procesar_datos, (void *)buffer) != 0) {
            perror("[ERROR]: Error al crear el hilo para el procesamiento de datos");
            close(client_sock);
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            return NULL;
        }

        // Esperar que el hilo termine antes de continuar
        pthread_join(pipe_thread, NULL); // Sincronización para esperar que el hilo termine
    }

    if (bytes_leidos == 0) {
        printf("[INFO]: Cliente desconectado.\n");
    } else if (bytes_leidos < 0) {
        perror("[ERROR]: Error al recibir datos del cliente");
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    close(client_sock);
    return NULL;
}

int main() {
    int client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Controlar la señal SIGINT (CTRL+C) para cerrar el servidor
    signal(SIGINT, handle_sigint);

    // Crear socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Enlazar el socket al puerto
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: Error al enlazar el socket");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
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

        // Crear un hilo para manejar el cliente
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
