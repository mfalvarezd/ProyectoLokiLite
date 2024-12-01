# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
AGENTE = agente
SERVIDOR = servidor
EJECUTAR_HYDRA = ejecutar_hydra  # Nueva variable para el ejecutable de hydra

# Regla por defecto
all: $(AGENTE) $(SERVIDOR) $(EJECUTAR_HYDRA)  # Agregar ejecutar_hydra a la regla all

# Compilar el agente
$(AGENTE): agente.o
	$(CC) $(CFLAGS) -o $(AGENTE) agente.o

# Compilar el servidor
$(SERVIDOR): servidor.o
	$(CC) $(CFLAGS) -o $(SERVIDOR) servidor.o

# Compilar el ejecutable de hydra
$(EJECUTAR_HYDRA): ejecutar_hydra.o
	$(CC) $(CFLAGS) -o $(EJECUTAR_HYDRA) ejecutar_hydra.o

# Regla para compilar el agente objeto
agente.o: agente.c
	$(CC) $(CFLAGS) -c agente.c

# Regla para compilar el servidor objeto
servidor.o: servidor.c
	$(CC) $(CFLAGS) -c servidor.c

# Regla para compilar el ejecutable de hydra objeto
ejecutar_hydra.o: ejecutar_hydra.c
	$(CC) $(CFLAGS) -c ejecutar_hydra.c

# Limpiar archivos generados
clean:
	rm -f $(AGENTE) $(SERVIDOR) $(EJECUTAR_HYDRA) *.o
