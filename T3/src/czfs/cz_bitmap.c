#include "cz_bitmap.h"

/* Funciones de bitmap */
int bitmap_get_free() {
  FILE* fp = fopen(ruta_bin, "rb+");
  char* b = calloc(2, sizeof(char));
  for(int i = 1024; i < 1024 * 8; i++) {
    fseek(fp, i, SEEK_SET);
    unsigned char byte[1];
    int byte_dec;
    fread(byte, 1, 1, fp);
    byte_dec = byte[0];

    char byte_bin[8];
    itoa(byte_dec, byte_bin, 2);

    char* filled_byte_bin;
    filled_byte_bin = fill_binario(byte_bin, 8);
    //printf("%s\n", filled_byte_bin);

    for(int j = 0; j < 8; j++) {
      if(filled_byte_bin[j] == '0') {
        free(filled_byte_bin);
        fclose(fp);
        free(b);

        return (i - 1024) * 8 + j + 1024;
      }
    }
    free(filled_byte_bin);
  }
  free(b);
  fclose(fp);

  return 0;
}

int bitmap_set_first() {
  FILE* fp = fopen(ruta_bin, "rb+");
  char* b = calloc(2, sizeof(char));

  for(int i = 1024; i < 1024 * 8; i++) {
    fseek(fp, i, SEEK_SET);

    unsigned char byte[1];
    int byte_dec;
    fread(byte, 1, 1, fp);
    byte_dec = byte[0];

    char byte_bin[8];
    itoa(byte_dec, byte_bin, 2);

    char* filled_byte_bin;
    filled_byte_bin = fill_binario(byte_bin, 8);

    for(int j = 0; j < 8; j++) {
      if(filled_byte_bin[j] == '0') {
        filled_byte_bin[j] = '1';

        int x = bin_to_dec(filled_byte_bin);
        b[0] = x;

        fseek(fp, i, SEEK_SET);
        fwrite(b, 1, 1, fp);

        free(filled_byte_bin);
        fclose(fp);
        free(b);

        return ((i - 1024) * 8) + j + 1024; //la posicion fisica exacta del bit
      }
    }
    free(filled_byte_bin);
  }
  free(b);
  fclose(fp);

  return 0;
}

bool bitmap_entry_is_free(int pos) {  // pos corresponde a una posicion en el disco
  if(pos >= 0 && pos <= 9)
    return false;

  if(pos >= 1024 && pos < 1024*8) {
    int offset = pos % 1024;
    int bloque = pos - offset;
    //printf("bloque %i offset %i\n", bloque, offset);

    FILE* fp = fopen(ruta_bin, "rb+");
    fseek(fp, bloque, SEEK_SET);
    unsigned char byte[1];
    int byte_dec;
    fread(byte, 1, 1, fp);
    byte_dec = byte[0];
    fclose(fp);

    char byte_bin[8];
    itoa(byte_dec, byte_bin, 2);

    char* filled_byte_bin;
    filled_byte_bin = fill_binario(byte_bin, 8);
    //printf("%s\n", filled_byte_bin);

    char val = filled_byte_bin[offset];
    free(filled_byte_bin);

    if(val == '1')
      return true;
    else
      return false;
  }
  return false;
}

int bitmap_de_bloque(int bloque) {
  int bloque_inicio = bloque - bloque%1024;
  int n_bloque = bloque_inicio/1024;

  return 1024 + n_bloque;
}

void bitmap_free(int bloque) {
  int direccion_bitmap_pos = bitmap_de_bloque(bloque);
  //printf("direccion bitmap pos %i\n", direccion_bitmap_pos);
  int n_bitmap_bloque = (direccion_bitmap_pos - 1024 - direccion_bitmap_pos % 1024) / 1024;  // de los 8 bitmap, cual?
  int n_bitmap_byte_con_offset = (direccion_bitmap_pos - (n_bitmap_bloque + 1) * 1024);
  int offset_bitmap = n_bitmap_byte_con_offset % 8;  // dentro del byte de bitmap, cual?
  int n_bitmap_byte = (n_bitmap_byte_con_offset - offset_bitmap) / 8;  // de los 1024 bytes para cada bitmap, cual?

  //printf("vamos a liberar %i, %i, %i\n", n_bitmap_bloque, n_bitmap_byte, offset_bitmap);

  FILE* fp = fopen(ruta_bin, "rb+");
  fseek(fp, (n_bitmap_bloque + 1) * 1024 + n_bitmap_byte, SEEK_SET);  // vamos al byte a modificar

  unsigned char byte[1];
  int byte_dec;
  fread(byte, 1, 1, fp);
  byte_dec = byte[0];

  char byte_bin[8];
  itoa(byte_dec, byte_bin, 2);

  char* filled_byte_bin;
  filled_byte_bin = fill_binario(byte_bin, 8);

  //printf("filled byte antes %s\n", filled_byte_bin);
  filled_byte_bin[offset_bitmap] = '0';
  //printf("filled byte despues %s\n", filled_byte_bin);

  int x = bin_to_dec(filled_byte_bin);
  free(filled_byte_bin);
  //printf("binario en dec %i\n", x);

  char* b = calloc(2, sizeof(char));
  b[0] = x;
  fseek(fp, (n_bitmap_bloque + 1) * 1024 + n_bitmap_byte, SEEK_SET);  // vamos al byte a modificar
  fwrite(b, 1, 1, fp);
  free(b);

  fclose(fp);
}
