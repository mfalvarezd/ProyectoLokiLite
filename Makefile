# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
AGENTE = agente
SERVIDOR = servidor

# Regla por defecto
all: $(AGENTE) $(SERVIDOR)

# Compilar el agente
$(AGENTE): agente.o
	$(CC) $(CFLAGS) -o $(AGENTE) agente.o

# Compilar el servidor
$(SERVIDOR): servidor.o
	$(CC) $(CFLAGS) -o $(SERVIDOR) servidor.o

# Regla para compilar el agente objeto
agente.o: agente.c
	$(CC) $(CFLAGS) -c agente.c

# Regla para compilar el servidor objeto
servidor.o: servidor.c
	$(CC) $(CFLAGS) -c servidor.c

# Limpiar archivos generados
clean:
	rm -f $(AGENTE) $(SERVIDOR) *.o
