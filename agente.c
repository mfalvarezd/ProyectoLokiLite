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
#define TIEMPO_ACTUALIZACION 5 // Tiempo de actualización constante
int keep_running = 1;

// CTRL+C PARA DETENER
void handle_sigint(int sig) {
    keep_running = 0;
}

//help
void print_usage(const char *prog_name) {
    printf("Uso: %s <servicio1> <servicio2> ... [servicioN]\n", prog_name);
}

//verificar parametro es un numero
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


int conectar_al_servidor() {
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[ERROR]: No se puede crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[ERROR]: Dirección IP no valida");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Manejar coneccion al servidor
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[ERROR]: No se puede conectar al servidor");
        close(sock);
        exit(EXIT_FAILURE); // en caso de falla se terminara la ejecucion
    }

    printf("[INFO]: Conexion satisfactoria con el servidor %s:%d\n", SERVER_IP, SERVER_PORT);
    return sock;
}


void ejecutarExec(const char *comando, char *resultado, size_t tamaño) {
    int pipefd[2]; //definir arreglo pipe 0 de lecutra 1 de escritura
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
        close(pipefd[0]); 
        close(pipefd[1]); 

        // Ejecutar el comando
        char *args[] = {"sh", "-c", (char *)comando, NULL}; // Para usar comandos de shell
        execv("/bin/sh", args);// se ejecutan los argumentos que reciva execv en forma de arreglo
        perror("[ERROR]: execv falló"); // el excv fallo
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        close(pipefd[1]); // Cerrar el lado de escritura del pipe

        // Leer del pipe
        ssize_t bytes_read = read(pipefd[0], resultado, tamaño - 1);
        if (bytes_read >= 0) {
            resultado[bytes_read] = '\0'; //la cadena existe y se pudo leer
        } else {
            perror("[ERROR]: No se pudo leer del pipe");
        }
        close(pipefd[0]); // Cerrar el lado de lectura del pipe
        wait(NULL); // Esperar a que el proceso hijo termine
    }
}

// Función de monitoreo y envio de datos al server
void monitorear_servicios(int server_sock, char **servicios, int num_servicios) {
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
                ejecutarExec(comando, resultado, sizeof(resultado));
                conteos[j] = atoi(resultado);
            }

            // Crear mensaje en formato JSON DASHBOARD
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

        sleep(TIEMPO_ACTUALIZACION); // Esperar el tiempo de actualización constante
    }
}

// verificar si solo recibe strrings como parametros
int es_nombre_servicio_valido(const char *servicio) {
    return !(es_numero(servicio)); // No debe ser un número
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

    // Verificar que todos los servicios sean válidos
    for (int i = 1; i < argc; i++) {
        if (es_numero(argv[i])) {
            fprintf(stderr, "[ERROR]: '%s' no es un nombre de servicio válido. No debe ser un número.\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Verificar que haya al menos 2 servicios
    int num_servicios = argc - 1;
    if (num_servicios < 2) {
        fprintf(stderr, "[ERROR]: Se requieren al menos dos servicios para monitorear.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char **servicios = argv + 1;

    printf("[INFO]: Numero de servicios a monitorear: %d\n", num_servicios);
    printf("[INFO]: Servicios a monitorear: ");
    for (int i = 0; i < num_servicios; i++) {
        printf("%s%s", servicios[i], (i < num_servicios - 1) ? ", " : "\n");
    }
    printf("[INFO]: Tiempo de actualizacion: %d segundos\n", TIEMPO_ACTUALIZACION);

    // Manejar la señal SIGINT para detener el programa
    signal(SIGINT, handle_sigint);

    // Conectar al servidor
    int server_sock = conectar_al_servidor();

    // Monitorear servicios y enviar datos al servidor
    monitorear_servicios(server_sock, servicios, num_servicios);

    // Cerrar el socket al finalizar
    close(server_sock);
    printf("[INFO]: Monitoreo detenido. Conexión cerrada.\n");

    return EXIT_SUCCESS;
}
