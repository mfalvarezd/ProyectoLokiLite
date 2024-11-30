#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <servicio1> <servicio2> [tiempo_actualizacion]\n", argv[0]);
        return 1;
    }

    // Servicios a monitorear
    char *servicio1 = argv[1];
    char *servicio2 = argv[2];

    // Tiempo de actualización
    int tiempo_actualizacion = 5; // Valor por defecto
    if (argc > 3) {
        tiempo_actualizacion = atoi(argv[3]);
    }

    printf("Monitoreando servicios: %s, %s\n", servicio1, servicio2);
    printf("Tiempo de actualización: %d segundos\n", tiempo_actualizacion);
     // Comando para journalctl del primer servicio
    char *args[] = {"journalctl", "-u", servicio1, "-n", "10", NULL};

    // Ejecuta el comando
    printf("Ejecutando comando para servicio: %s\n", servicio1);
    if (execvp(args[0], args) == -1) {
        perror("Error ejecutando execvp");
        return 1;
    }


    return 0;
}
