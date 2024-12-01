#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SERVER_IP "127.0.0.1"  // Dirección IP del servidor
#define SERVER_PORT 8080       // Puerto del servidor
#define BUFFER_SIZE 1024       // Tamaño del buffer para enviar datos
#define TIEMPO_ACTUALIZACION_DEFAULT 5 
int keep_running = 1;

// Función para manejar la señal SIGINT y detener el programa
void handle_sigint(int sig) {
    keep_running = 0;
}

// Función para imprimir el uso correcto del programa
void print_usage(const char *prog_name) {
    printf("Uso: %s <servicio1> <servicio2> ... [servicioN] [tiempo_actualizacion]\n", prog_name);
}

// Función para verificar si una cadena es un número válido
int es_numero(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int es_entero_mayor_a_cero(const char *str) {
    // Verifica si la cadena es un entero mayor a cero
    int num = atoi(str);
    return (num > 0);
}

// Función para conectarse al servidor
int conectar_al_servidor() {
    int sock;
    struct sockaddr_in server_addr;

    // Crear el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[ERROR]: Dirección IP no válida");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: No se pudo conectar al servidor");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[INFO]: Conexión establecida con el servidor %s:%d\n", SERVER_IP, SERVER_PORT);
    return sock;
}

// Función para ejecutar un comando y obtener su resultado
void ejecutar_comando_y_obtener_resultado(const char *comando, char *resultado, size_t tamaño) {
    int pipefd[2]; // Array para el pipe
    pid_t pid;

    // Crear un pipe
    if (pipe(pipefd) == -1) {
        perror("[ERROR]: No se pudo crear el pipe");
        return;
    }

    // Crear un nuevo proceso
    pid = fork();
    if (pid == -1) {
        perror("[ERROR]: No se pudo hacer fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0) { // Proceso hijo
        // Redirigir la salida estándar al pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // Cerrar el lado de lectura del pipe
        close(pipefd[1]); // No necesitamos el lado de escritura en el hijo

        // Ejecutar el comando
        char *args[] = {"sh", "-c", (char *)comando, NULL}; // Para usar comandos de shell
        execv("/bin/sh", args);
        perror("[ERROR]: execv falló"); // Solo se llega aquí si execv falla
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        close(pipefd[1]); // Cerrar el lado de escritura del pipe

        // Leer del pipe
        ssize_t bytes_read = read(pipefd[0], resultado, tamaño - 1);
        if (bytes_read >= 0) {
            resultado[bytes_read] = '\0'; // Asegurarse de que la cadena esté terminada en null
        } else {
            perror("[ERROR]: No se pudo leer del pipe");
        }
        close(pipefd[0]); // Cerrar el lado de lectura del pipe
        wait(NULL); // Esperar a que el proceso hijo termine
    }
}

// Función para monitorear servicios y enviar datos al servidor
void monitorear_servicios(int server_sock, char **servicios, int num_servicios, int tiempo_actualizacion) {
    char buffer[BUFFER_SIZE];
    char resultado[BUFFER_SIZE];

    // Array de prioridades
    const char *prioridades[] = {"alert", "err", "warning", "info"};
    int num_prioridades = sizeof(prioridades) / sizeof(prioridades[0]);

    while (keep_running) {
        for (int i = 0; i < num_servicios; i++) {
            printf("[INFO]: Monitoreando servicio: %s\n", servicios[i]);

            // Inicializar conteos de prioridades
            int conteos[num_prioridades];
            memset(conteos, 0, sizeof(conteos)); // Reiniciar conteos

            // Contar las diferentes prioridades
            for (int j = 0; j < num_prioridades; j++) {
                char comando[BUFFER_SIZE];
                snprintf(comando, sizeof(comando), "journalctl -u %s -p '%s' | wc -l", servicios[i], prioridades[j]);
                ejecutar_comando_y_obtener_resultado(comando, resultado, sizeof(resultado));
                conteos[j] = atoi(resultado);
            }

            // Crear mensaje en formato JSON
            snprintf(buffer, sizeof(buffer),
                     "{ \"servicio\": \"%s\", \"alertas\": %d, \"errores\": %d, \"avisos\": %d, \"informacion\": %d }",
                     servicios[i], conteos[0], conteos[1], conteos[2], conteos[3]);

            // Enviar los datos al servidor
            if (send(server_sock, buffer, strlen(buffer), 0) == -1) {
                perror("[ERROR]: No se pudieron enviar los datos al servidor");
            } else {
                printf("[INFO]: Datos enviados: %s\n", buffer);
            }
        }

        sleep(tiempo_actualizacion); // Esperar el tiempo de actualización
    }
}

int main(int argc, char *argv[]) {
    // Verificar si hay al menos 2 servicios
    if (argc < 3) { // Al menos 2 servicios + el nombre del programa
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Aseguramos que al menos los dos primeros argumentos sean servicios
    char *servicio1 = argv[1];
    char *servicio2 = argv[2];

    // Inicializamos la variable tiempo de actualización
    int tiempo_actualizacion = TIEMPO_ACTUALIZACION_DEFAULT;
    int num_servicios = argc - 1; // Inicialmente, todos los argumentos se consideran servicios
    if (argc > 3) { // Verificar si hay un cuarto argumento para tiempo
        if (es_entero_mayor_a_cero(argv[argc - 1])) { // Solo verificamos el último argumento
            tiempo_actualizacion = atoi(argv[argc - 1]);
            num_servicios--; // Reducimos el conteo de servicios, ya que el último argumento es tiempo
        } else {
            printf("El último parámetro debe ser un entero mayor a 0.\n");
            return EXIT_FAILURE;
        }
    }

    // Verificar que haya al menos 2 servicios después de considerar el tiempo de actualización
    if (num_servicios < 2) {
        fprintf(stderr, "[ERROR]: Se requieren al menos dos servicios para monitorear.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char **servicios = argv + 1; // Los servicios comienzan en argv[1]

    printf("[INFO]: Número de servicios a monitorear: %d\n", num_servicios);
    printf("[INFO]: Servicios a monitorear: ");
    for (int i = 0; i < num_servicios; i++) {
        printf("%s%s", servicios[i], (i < num_servicios - 1) ? ", " : "\n");
    }
    printf("[INFO]: Tiempo de actualización: %d segundos\n", tiempo_actualizacion);

    // Manejar la señal SIGINT para detener el programa
    signal(SIGINT, handle_sigint);

    // Conectar al servidor
    int server_sock = conectar_al_servidor();

    // Monitorear servicios y enviar datos al servidor
    monitorear_servicios(server_sock, servicios, num_servicios, tiempo_actualizacion);

    // Cerrar el socket al finalizar
    close(server_sock);
    printf("[INFO]: Monitoreo detenido. Conexión cerrada.\n");

    return EXIT_SUCCESS;
}
