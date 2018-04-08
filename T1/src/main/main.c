#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "colas.h"

void stats(int sig) {
		printf("AQUI TIENEN QUE IMPRIMIRSE LAS STATS. TAMBIEN HAY QUE VER QUE SE RETORNE 0\n");
    //close(0);  // foo is displayed if this line is commented out
    _Exit(0);
}

int main(int argc, char *argv[])
{
	/* El programa recibe 4 parametros (por ahora, despues hay que agregar el de la v3) */
	if(argc != 5 && argc != 6)
	{
		printf("Modo de uso: %s <version> <file> <quantum> <queues> [<S>]\n", argv[0]);
		return 1;
	}

	char* version = argv[1];

	if(argc == 5 && (strcmp(version,"v1") != 0))
	{
		printf("Modo de uso incorrecto: con 4 parametros deberia pedir la v1\n");
		return 1;
	}
	if(argc == 6 && ((strcmp(version,"v2") != 0) && ((strcmp(version,"v3") != 0))))
	{
		printf("Modo de uso incorrecto: con 5 parametros deberia pedir la v2 o v3\n");
		return 1;
	}

	char* input_file = argv[2];
  int quantum = atoi(argv[3]);
  int queues = atoi(argv[4]);
	FILE *archivo_procesos;
	archivo_procesos = fopen(input_file, "r");	//leyendo archivo de input
	int S;
	if (argc == 6) {
		S = atoi(argv[5]); //guardar S solo si estamos en v2 o v3
		printf("S: %i\n", S);
	}
	if (!(archivo_procesos)) {
		printf("Archivo %s no existe\n", input_file);
		return 1;
	}

	Queue *cola_por_nacer = ConstructQueue(-1, -1);


	signal(SIGINT, stats);  // por si se hace ctrl + c

	char nombre_proceso[255];
	int tiempo_inicio;
	int cantidad_valores;
	int valor_actual;
	int pid = 0;
	while (fscanf(archivo_procesos, "%s", nombre_proceso) != EOF) {
		Proceso *proceso = malloc(sizeof(Proceso));

		proceso->PID = pid++;
		strcpy(proceso->nombre, nombre_proceso);
		fscanf(archivo_procesos, "%i", &tiempo_inicio);
		proceso->prioridad = tiempo_inicio;
		proceso->waiting_time = -tiempo_inicio; // para que se cumpla finish_time - prioridad - sum(linea de tiempo)
		fscanf(archivo_procesos, "%i", &cantidad_valores);
		proceso->quantum_restante = 0;

		Time_Queue *linea_tiempo = ConstructTimeQueue();

		for(int i = 0; i < cantidad_valores; i++) {
			fscanf(archivo_procesos, "%i", &valor_actual);
			TimeEnqueue(linea_tiempo, valor_actual);
			proceso->waiting_time -= valor_actual;  // quedara con -prioridad-sum(linea_de_tiempo)
		}

		proceso->linea_de_tiempo = linea_tiempo;

		proceso->n_veces_cpu = 0;
		proceso->n_veces_int = 0;
		proceso->finish_time = 0;
		proceso->response_time = 0;
		proceso->response_time_setted = FALSE;

		Ordered_Enqueue(cola_por_nacer, proceso);
	}
	//Print_Queue(cola_por_nacer);
	fclose(archivo_procesos);

	Queue_Queue *colas = ConstructMLFQueue(queues, quantum, !strcmp(version,"v3"));
	Queue *cola_terminados = ConstructQueue(-1, -1);
	int cantidad_procesos = cola_por_nacer->size;

	int T = 0;
	while(TRUE) {
		if(cola_terminados->size >= cantidad_procesos) break;

		Proceso *nacer = Born(*cola_por_nacer, T);
		if(nacer != NULL) {
			Dequeue(cola_por_nacer);
			Enqueue(colas->head, nacer);
			nacer->estado = READY;
			printf("Proceso %s nace en t = %i\n", nacer->nombre, nacer->prioridad);
		}

		Ejecutar_proceso(colas, cola_terminados, T);

		if (((strcmp(version,"v2") == 0) || (strcmp(version,"v3") == 0)) && T%S == 0) {
			Aging(colas);
		}

		T++;
	}
	T--; // pq hay que eliminar un t++ que no se ejecuto para estadisticas

	printf("\n");
	printf("Procesos terminados: %i\n", cola_terminados->size);
	printf("Tiempo total: %i\n", T);
	//Queue_Print_Queue(colas);
	printf("---- Cola Terminados ---\n");
	Print_Queue(cola_terminados);

	sleep(10);  // eliminar esto, es solo para probar ctrl + c
	stats(0);  // si no se hizo ctrl + c display stats

	return 0;
}
