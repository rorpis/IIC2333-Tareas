#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "funciones.h"

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

int bin_to_dec(char* bin) {
	int dec = 0;
	char c;

	for(int i = 0; i < strlen(bin); i++) {
		c = bin[i];
		if(c == '1') dec = dec * 2 + 1;
		else if(c == '0') dec *= 2;
	}

	return dec;
}

char* fill_binario(char* binario, int cantidad) {
	char* final = calloc(cantidad + 1, sizeof(char));  // calloc por que hay q inicializar

	for(int i = 0; i < cantidad - (int)strlen(binario); i++) strcat(final, "0");
	strcat(final, binario);

	return final;
}

char* cut_string(char* string, int inicio, int final) {
	char* string_final = malloc(sizeof(char) * (final - inicio + 1));
	for(int i = 0; i < (final - inicio); i++) {
		string_final[i] = string[inicio + i];
	}
	string_final[final - inicio] = '\0';

	return string_final;
}

char* leer_bin(char *filename, int pos) {
  FILE* fp;
  char* frame = malloc(sizeof(char) * 256);
  fp = fopen(filename, "rb");
  fseek(fp, pos * 256, SEEK_SET);
  fread(frame, 1, 256, fp);
  fclose(fp);

	return frame;
}
