#include "cz_API.h"

czFILE* cz_open(char* filename, char mode) {
  czFILE* file = NULL;
  if(mode == 'r') {
    FILE* fp = fopen(ruta_bin, "rb");

    int i = 0;
    while(i < 1024) {
      char valid[1];
      fread(valid, 1, 1, fp);
      char name[11];
      fread(name, 11, 1, fp);
      char indice[4];
      fread(indice, 4, 1, fp);

      if(atoi(valid) && strcmp(name, filename) == 0) {  // falta chequear si el bitmap esta ocupado // si existe
        file = malloc(sizeof(czFILE));

        file->direccion_directorio = i;
        file->direccion_bloque = atoi(indice);

        file->nombre = malloc(sizeof(char) * 11);
        strcpy(file->nombre, name);

        fseek(fp, atoi(indice), SEEK_SET);  // nos vemos al archivo
        char tamano[4];
        fread(tamano, 4, 1, fp);
        file->tamano = atoi(tamano);
        char creacion[4];
        fread(creacion, 4, 1, fp);
        file->creacion = atoi(creacion);
        char modificacion[4];
        fread(modificacion, 4, 1, fp);
        file->modificacion = atoi(modificacion);
        char next_bloque[4];
        fseek(fp, 1008, SEEK_CUR);
        fread(next_bloque, 4, 1, fp);  // nos saltamos la data
        file->next_bloque = atoi(next_bloque);

        fclose(fp);

        return file;
      }

      i += 16;
    }

    fclose(fp);

  }
  if(mode == 'w') {
    file = malloc(sizeof(czFILE));
    file->nombre = malloc(sizeof(char) * 11);
    strcpy(file->nombre, filename);

    // falta direccion de los demas dependiendo de la direccion disponible en memoria
  }

  return file;
}

int cz_exists(char* filename) {
  FILE* fp = fopen(ruta_bin, "rb");

  int i = 0;
  while(i < 1024) {
    char valid[1];
    fread(valid, 1, 1, fp);
    char name[11];
    fread(name, 11, 1, fp);
    char indice[4];
    fread(indice, 4, 1, fp);

    if(atoi(valid) && strcmp(name, filename) == 0) {  // falta chequear si el bitmap esta ocupado
      fclose(fp);
      return 1;
    }

    i += 16;
  }

  fclose(fp);
  return 0;
}

void cz_ls() {
  FILE* fp = fopen(ruta_bin, "rb");

  printf("Haciendo ls de %s\n", ruta_bin);

  int i = 0;
  while(i < 1024) {
    char valid[1];
    fread(valid, 1, 1, fp);
    char name[11];
    fread(name, 11, 1, fp);
    char indice[4];
    fread(indice, 4, 1, fp);

    if(atoi(valid))
      printf("%s\n", name);

    i += 16;
  }

  fclose(fp);
}



/* Funciones de bitmap */
char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char* fill_binario(char* binario, int cantidad) {
	char* final = calloc(cantidad + 1, sizeof(char));  // calloc por que hay q inicializar

	for(int i = 0; i < cantidad - (int)strlen(binario); i++) strcat(final, "0");
	strcat(final, binario);

	return final;
}

int bitmap_get_free() {
  FILE* fp = fopen(ruta_bin, "rb");

  for (int i = 1024; i < (1024*8); i++) {
    fseek(fp, i, SEEK_SET);
    char occupied[1];
    fread(occupied, 1, 1, fp);
    printf("%i\n", occupied[0]);

    char* occupied_bin = calloc(9, sizeof(char));
    itoa(atoi(occupied), occupied_bin, 2);
    char* fill_occupied = fill_binario(occupied_bin, 8);
    printf("%i: %s\n", i, fill_occupied);
    free(occupied_bin);

    /*for(int j = 7; j >= 0; j--) {
      //printf("%c\n", fill_occupied[j]);
      if(fill_occupied[j] == '0') {
        free(fill_occupied);
        fclose(fp);
        return ((i - 1024) * 8) + j + 1024;  // esta es la pos del disco duro
      }
    }*/
    if(i == 1026) break;
    //free(fill_occupied);
  }

  fclose(fp);
  return 0;
}
