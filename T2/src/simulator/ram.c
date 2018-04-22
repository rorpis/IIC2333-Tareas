#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "funciones.h"
#include "ram.h"

char*** crear_ram() {
  char ***TLB;
  TLB = malloc(sizeof(char***) * 64);
  for (int i = 0; i < 256; i++) {
    TLB[i] = malloc(sizeof(char**) * 2);
    TLB[i][0] = "-";  //frame completo
    TLB[i][1] = "-";  //time
  }
  return TLB;
};

int insertar_en_ram(char*** RAM, char* frame, int tiempo) {
  int insertado = 0;
  int n_frame = 0;
  for (int i = 0; i < 64; i++) {
    if (strcmp(RAM[i][0], "-")) { //ver si esta vacio
      RAM[i][0] = frame;
      itoa(tiempo, RAM[i][1], 10);
      insertado = 1;
      n_frame = i;
    }
  }
  if (insertado == 0) { //todos ocupados
    double min = +INFINITY;
    int valor;
    int i_min;
    for (int i = 0; i < 64; i++) {
      valor = atoi(RAM[i][1]);
      if (valor < min) {
        min = valor;
        i_min = i;
      }
    }
    //hacer swap out de RAM[i_min] y marcarlo en su PTE con 100
    RAM[i_min][0] = frame;
    itoa(tiempo, RAM[i_min][1], 10);
    n_frame = i_min;
  }
  return n_frame;
}
