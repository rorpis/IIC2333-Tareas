#include "cz_API.h"

/* Funciones de tarea */
czFILE* cz_open(char* filename, char mode) {
  czFILE* file = NULL;
  if(mode == 'r') {
    FILE* fp = fopen(ruta_bin, "rb");

    int i = 0;
    while(i < 1024) {
      char* valid = calloc(2, sizeof(char));
      fread(valid, 1, 1, fp);
      char* name = calloc(12, sizeof(char));
      fread(name, 11, 1, fp);
      int indice;
      fread(&indice, 4, 1, fp);

      if(valid[0] == 1 && strcmp(name, filename) == 0 && !bitmap_entry_is_free(indice)) { // si existe
        file = malloc(sizeof(czFILE));

        file->indice_en_directorio = i;
        file->direccion_bloque = indice * 1024; //direccion del bloque indice

        file->nombre = name;

        printf("indice en directorio: %i\n", indice);
        fseek(fp, file->direccion_bloque, SEEK_SET);  // nos vemos al archivo
        int tamano;
        fread(&tamano, 4, 1, fp);
        file->tamano = tamano;

        fread(&file->creacion, 4, 1, fp);
        fread(&file->modificacion, 4, 1, fp);

        char* next_bloque = calloc(5, sizeof(char));
        fseek(fp, 1008, SEEK_CUR);
        fread(next_bloque, 4, 1, fp);  // nos saltamos la data
        file->next_bloque = atoi(next_bloque);
        free(next_bloque);
        //fclose(fp);
        file->modo = 'r';
        file->closed = 0;

        printf("tamano archivo: %i\n", file->tamano);
        int tamano_datos = file->tamano - 12; //incluye los punteros
        if (tamano_datos == 0) {
          file->tamano_datos = tamano_datos;
        }
        else {
          int cantidad_bloques = ((tamano_datos - 1) / 1028) + 1;
          printf("cantidad de bloques: %i\n", cantidad_bloques);
          file->tamano_datos = tamano_datos - (4 * cantidad_bloques);
        }
        printf("---tamano datos: %i\n", file->tamano_datos);
        fclose(fp);
        free(valid);
        return file;
      }
      i += 16;
      free(name);
      free(valid);
    }
    fclose(fp);
  }

  if(mode == 'w') {
    //ver si existe, y abrirlo en modo w y retornar NULL
    if (cz_exists(filename))
      return NULL;

    FILE* fp = fopen(ruta_bin, "rb+");

    int i = 0;
    while(i < 1024) { //iteramos en el dir
      char valid[1];
      fseek(fp, i, SEEK_SET);
      fread(valid, 1, 1, fp);
      if(valid[0] == 0) { // si hay espacio libre en esta parte del dir
        fseek(fp, i, SEEK_SET);  // devolvemos para escribir en el directorio

        char* valid_ch = calloc(2, sizeof(char));
        valid_ch[0] = 1;
        fwrite(valid_ch, 1, 1, fp);  // ahora es valid
        free(valid_ch);

        char* filename_11 = calloc(12, sizeof(char));
        strcpy(filename_11, filename);
        fwrite(filename_11, 11, 1, fp);  // guardamos el name
        free(filename_11);

        int n_bloque = bitmap_set_first() - 1024; // setea en bitmap el bloque a usar y se guarda la posicion en disco asignada
        fwrite((const void*) &n_bloque, 4, 1, fp);

        /* Nos metemos al bloque del archivo */
        fseek(fp, n_bloque * 1024, SEEK_SET);

        int tamano = 12;  // 12 bytes de metadata
        fwrite((const void*) &tamano, 4, 1, fp);

        time_t t_creacion = time(NULL);
        fwrite((const void*) &t_creacion, 4, 1, fp);  // T creacion
        fwrite((const void*) &t_creacion, 4, 1, fp);  // T modificacion

        file = malloc(sizeof(czFILE));
        file->indice_en_directorio = i;
        file->direccion_bloque = n_bloque * 1024;
        file->nombre = malloc(sizeof(char) * 11);
        strcpy(file->nombre, filename);
        file->modo = 'w';
        file->closed = false;
        file->tamano = 12; //solamente el metadata
        file->tamano_datos = 0; // 0 punteros escritos
        file->creacion = t_creacion;
        file->modificacion = t_creacion;

        printf("archivo %s creado en bloque %i\n", filename, n_bloque);

        fclose(fp);

        return file;
      }
      i += 16;
    }

    fclose(fp);
  }

  return file; //directorio ya esta lleno, no se como poner otra cosa que no sea NULL para decir esto
}

int cz_exists(char* filename) {
  FILE* fp = fopen(ruta_bin, "rb");

  int i = 0;
  while(i < 1024) {  // 1024 son de directorio
    char valid[1];
    fread(valid, 1, 1, fp);
    char name[11];
    fread(name, 11, 1, fp);
    int indice;
    fread(&indice, sizeof(int), 1, fp);

    if(valid[0] == 1 && strcmp(name, filename) == 0 && !bitmap_entry_is_free(indice)) {
      fclose(fp);

      return 1;
    }

    i += 16; //avanza al siguiente bloque
  }
  fclose(fp);

  return 0;
}

/*int cz_write(czFILE* file_desc, void* buffer, int nbytes) {
  if(file_desc->modo == 'w' && !file_desc->closed) {
    int bytes_escribir = MIN((252 + 256) * 1024 - file_desc->tamano_datos, nbytes);  // maximo que puede escribir
    printf("vamos a escribir %i bytes\n", bytes_escribir);

    int tamano_restante_ultimo_bloque = file_desc->tamano_datos % 1024;
    int cantidad_bloques_nuevos = (bytes_escribir - tamano_restante_ultimo_bloque)/1024;
    if(file_desc->tamano_datos == 0)
      cantidad_bloques_nuevos += 1;
    printf("tamano restante %i, bloques nuevos %i\n", tamano_restante_ultimo_bloque, cantidad_bloques_nuevos);

    file_desc->tamano += bytes_escribir + 4 * cantidad_bloques_nuevos;
    time_t t_modificacion = time(NULL);
    file_desc->modificacion = t_modificacion;
    int modificacion_int = (int)file_desc->modificacion;

    FILE* fp = fopen(ruta_bin, "rb+");

    fseek(fp, file_desc->direccion_bloque, SEEK_SET);
    int tamano = file_desc->tamano;
    printf("%i\n", tamano);
    fwrite((const void*) &tamano, 4, 1,fp);
    fseek(fp, 4, SEEK_CUR);  // nos saltamos la creacion
    // ESTA MALO esto de abajo
    fwrite((const void*) &modificacion_int, 4, 4, fp);  // T modificacion

    printf("%s\n", buffer);
    int n_bloque = (file_desc->tamano_datos - tamano_restante_ultimo_bloque)/1024;
    int direccion_bloque;
    int bytes_escritos;
    int sum_bytes_escritos = 0;
    void* buffer_sobra = buffer;
    while(bytes_escribir > 0) {  // en realidad != 0
      printf("faltan %i bytes para terminar\n", bytes_escribir);
      if(n_bloque <= 251)  // del 0 al 251 va en el espacio de 1008 bytes
        direccion_bloque = file_desc->direccion_bloque + 12 + n_bloque * 4;
      else // del 252 en adelante van en direccionamiento indirecto
        direccion_bloque = file_desc->next_bloque + (n_bloque - 251) * 4;  // le sumo los 1008 + el inicio del otro bloque

      if(tamano_restante_ultimo_bloque > 0) {  // rellenamos ultimo bloque
        //bytes_escritos = cz_write_bloque(direccion_bloque, buffer_sobra, tamano_restante_ultimo_bloque);
        bytes_escritos = tamano_restante_ultimo_bloque;
        fseek(fp, direccion_bloque, SEEK_SET);
        int direccion_bloque_dato = (bitmap_set_first() - 1024) * 1024;
        fwrite((const void*) & direccion_bloque_dato,sizeof(int),1,fp);
        fseek(fp, direccion_bloque_dato, SEEK_SET);
        fwrite(buffer_sobra, tamano_restante_ultimo_bloque, 1, fp);
      } else {  //nuevo bloque
        int bytes_escribir_bloque_n = MIN(1024, bytes_escribir);
        printf("ddddd %i\n", bytes_escribir_bloque_n);
        //bytes_escritos = cz_write_bloque(direccion_bloque, buffer_sobra, bytes_escribir_bloque_n);
        bytes_escritos = bytes_escribir_bloque_n;
        fseek(fp, direccion_bloque, SEEK_SET);
        int direccion_bloque_dato = (bitmap_set_first() - 1024) * 1024;
        printf("jfslkdjflk %i\n", direccion_bloque_dato);
        fwrite((const void*) & direccion_bloque_dato,sizeof(int),1,fp);
        fseek(fp, direccion_bloque_dato, SEEK_SET);
        fwrite(buffer_sobra, bytes_escribir_bloque_n, 1, fp);
      }
      //buffer_sobra = buffer_desde(buffer, bytes_escritos);
      sum_bytes_escritos += bytes_escritos;
      bytes_escribir -= bytes_escritos;
      file_desc->tamano_datos += bytes_escritos;
      tamano_restante_ultimo_bloque -= bytes_escritos;
      if(tamano_restante_ultimo_bloque == 0) {
        n_bloque++;
        tamano_restante_ultimo_bloque = 1024;
      }
      if(n_bloque == 251) {
        file_desc->tamano += 4;
        // hacer fwrite del tamano
        //file_desc->direccion_bloque
        // hacer fwrite del bloque
        printf("hay que setear direccion para direccionamiento indirecto y incrementar tamaño del archivo\n");
      }
    }
    // hacer fwrite del nuevo tamaño

    fclose(fp);

    return sum_bytes_escritos;  // cambiar a retornar bytes escritos
  }
  return -1;
}*/

void cz_ls() {
  FILE* fp = fopen(ruta_bin, "rb");

  printf("Printeando ls\n");

  int i = 0;
  while(i < 1024) {
    char* valid = calloc(2, sizeof(char));
    fread(valid, 1, 1, fp);
    char* name = calloc(12, sizeof(char));
    fread(name, 11, 1, fp);
    int indice;
    fread(&indice, sizeof(int), 1, fp);

    if(valid[0] == 1 && !bitmap_entry_is_free(indice))
      printf("%s\n", name);

    free(valid);
    free(name);

    i += 16;
  }

  fclose(fp);
}

void cz_mount(char* diskfileName) {
  ruta_bin = diskfileName;

  for(int k = 0; k < 9; k++) {  // los primeros 9 son directorio y bitmap
    bitmap_set_first();
  }
}

int cz_close(czFILE* file_desc) {
  file_desc->closed = true;
  return 0;
}

int cz_mv(char* orig, char *dest) {
  if (!cz_exists(orig) || cz_exists(dest)) {  // ver si orig existe y si dest no existe
    return 1;
  }

  // en caso de que dest no existe y orig si existe:
  FILE* fp = fopen(ruta_bin, "rb+");

  int i = 0;
  while(i < 1024) {
    char* valid = calloc(2, sizeof(char));
    fread(valid, 1, 1, fp);
    char* name = calloc(12, sizeof(char));
    fread(name, 11, 1, fp);
    int indice;
    fread(&indice, sizeof(int), 1, fp);

    if(valid[0] == 1 && !bitmap_entry_is_free(indice) && (strcmp(orig, name) == 0)){
      printf("antiguo: '%s', cambiado a: '%s'\n", orig, dest);
      fseek(fp, i + 1, SEEK_SET);  // nos movemos al nombre
      fwrite(dest, 11, 1, fp);

      free(valid);
      free(name);
      fclose(fp);

      return 0;
    }

    free(valid);
    free(name);

    i += 16;
  }

  fclose(fp);
  return 0;
}

int cz_write(czFILE* file_desc, void* buffer, int nbytes) {
  printf("entrando a escribir\n");
  if(file_desc->modo == 'w' && !file_desc->closed) {
    int bytes_escribir = MIN((252 + 256) * 1024 - file_desc->tamano_datos, nbytes);  // maximo que puede escribir
    printf("vamos a escribir %i bytes\n", bytes_escribir);

    int tamano_restante_ultimo_bloque = 1024 - file_desc->tamano_datos % 1024;
    printf("tamaño restante en ultimo bloque: %i\n", tamano_restante_ultimo_bloque);

    int cantidad_bloques_nuevos = (bytes_escribir - tamano_restante_ultimo_bloque) / 1024;
    if(tamano_restante_ultimo_bloque == 1024)  // si el ultimo bloque lo ocupamos 100%, entonces usaremos uno nuevo
      cantidad_bloques_nuevos += 1;
    printf("tamano restante ultimo bloque %i, bloques nuevos %i\n", tamano_restante_ultimo_bloque, cantidad_bloques_nuevos);

    printf("tamano archivo antes: %i\n", file_desc->tamano);
    file_desc->tamano += (4 * cantidad_bloques_nuevos) + bytes_escribir; // no se considera fragmentacion interna
    printf("tamano archivo despues: %i\n", file_desc->tamano);

    int n_bloque = (file_desc->tamano_datos - tamano_restante_ultimo_bloque) / 1024;
    if(tamano_restante_ultimo_bloque == 1024)  // si el ultimo bloque lo ocupamos 100%, entonces vamos a otro
      n_bloque += 1;
    printf("vamos a escribir desde el bloque %i\n", n_bloque);

    printf("bytes a escribir: %i\n", bytes_escribir);
    printf("tamano solo datos antes: %i\n", file_desc->tamano_datos);
    file_desc->tamano_datos += bytes_escribir;
    printf("tamano solo datos despues: %i\n", file_desc->tamano_datos);

    time_t t_modificacion = time(NULL);
    file_desc->modificacion = t_modificacion;
    printf("t modificacion %i\n", (int)file_desc->modificacion);

    FILE* fp = fopen(ruta_bin, "rb+");

    printf("escribimos tamano %i y t modificacion %i\n", file_desc->tamano, (int)file_desc->modificacion);
    fseek(fp, file_desc->direccion_bloque, SEEK_SET);  // vamos a la direcion en directorio
    fwrite((const void*) &file_desc->tamano, 4, 1,fp);
    fseek(fp, 4, SEEK_CUR);  // nos saltamos la creacion
    fwrite((const void*) &file_desc->modificacion, 4, 1, fp);  // T modificacion

    printf("escibiendo: %s\n", buffer);
    int direccion_bloque;
    if(n_bloque < 251)  // si escribimos en los bloques directos
      direccion_bloque = file_desc->direccion_bloque + 12 + (n_bloque * 4);
    else  // si escribimos en los bloques indirectos | si next_bloque no esta seteado se setea cuando se entra al while y se recalcula direccion_bloque
      direccion_bloque = file_desc->next_bloque + ((n_bloque - 251) * 4);
    printf("direccion bloque datos a escribir: %i\n", direccion_bloque);

    int bytes_restantes = nbytes;
    int sum_bytes_escritos = 0;
    while(bytes_restantes > 0 && n_bloque < 508) {  // el primero va de 0 a 251 y el segundo de 252 a 252 + 256 = 508
      if(n_bloque >= 252 && file_desc->next_bloque == 0) {  // seteamos la direccion de punteros indirectos
        file_desc->next_bloque = (bitmap_set_first() - 1024) * 1024;
        direccion_bloque = file_desc->next_bloque + ((n_bloque - 251) * 4);

        fseek(fp, file_desc->direccion_bloque, SEEK_SET);  // vamos al tamano del archivo
        file_desc->tamano += 4;  // le sumamos los bytes de direccion del bloque indirecto
        fwrite((const void*) &file_desc->tamano, 4, 1, fp);

        fseek(fp, 12 - 4 + 1008, SEEK_CUR);  // vamos a la direccion del bloque indirecto
        fwrite((const void*) &file_desc->next_bloque, 4, 1, fp);
        printf("Guardando direccion de bloque indirecto. Nuevo tamano archivo %i, direccion %i\n", file_desc->tamano, (int)file_desc->next_bloque);
      }

      int direccion_bloque_datos;
      fseek(fp, direccion_bloque, SEEK_SET);  // nos vamos al bloque
      fread(&direccion_bloque_datos, 4, 1, fp);  // y leemos que bloque tiene asignado

      if((tamano_restante_ultimo_bloque == 1024 && bitmap_entry_is_free(direccion_bloque_datos))
        || (direccion_bloque_datos >= 0 && direccion_bloque_datos <= 9)) {  // tenemos que hacer una nueva entrada si es que no existe en bitmap o tiene asignado los reservados
        direccion_bloque_datos = (bitmap_set_first() - 1024) * 1024;
        fseek(fp, direccion_bloque, SEEK_SET);  // nos vamos al bloque
        fwrite((const void*) &direccion_bloque_datos, 4, 1, fp);  // escribimos el bloque asignado
      }

      int bytes_a_escribir = MIN(tamano_restante_ultimo_bloque, bytes_restantes);
      fseek(fp, direccion_bloque_datos, SEEK_SET);  // nos posicionamos en el bloque a escribir
      printf("Vamos a escribir %i bytes en %i\n", bytes_a_escribir, direccion_bloque_datos);

      fseek(fp, 1024 - tamano_restante_ultimo_bloque, SEEK_CUR);  // movemos al puntero a lo que queda
      printf("movemos el puntero a %i\n", 1024 - tamano_restante_ultimo_bloque);

      //escribiendo aca
      fwrite(buffer, bytes_a_escribir, 1, fp);
      printf("antigu buffer: %s\n", buffer);
      buffer = buffer + bytes_a_escribir;
      printf("nuevo buffer: %s\n", buffer);
      //

      tamano_restante_ultimo_bloque -= bytes_a_escribir;
      sum_bytes_escritos += bytes_a_escribir;  // correctamente deberia ir en el retorno de fwrite, pero no calza nose pq
      bytes_restantes -= bytes_a_escribir;
      printf("bytes restantes: %i\n", bytes_restantes);
      printf("sum bytes escritos %i\n", sum_bytes_escritos);
    }

    fclose(fp);

    return sum_bytes_escritos;  // retornando los bytes que se escribieron
  }
  return -1;
}


int cz_read(czFILE* file_desc, void* buffer, int nbytes){
  if (file_desc->modo == 'w' || file_desc->closed) {
    return -1;
  }

  int bytes_leer;
  bytes_leer = MIN(file_desc->tamano_datos, nbytes);

  printf("entrando a leer\n");
  int sum_bytes_leidos = 0;
  int datos_restantes = file_desc->tamano_datos;  //del archivo entero
  if(file_desc->modo == 'r' && !file_desc->closed) {  //porque open read no esta listo
    printf("vamos a leer %i bytes\n", bytes_leer);
    int direccion_bloque_actual = file_desc->direccion_bloque + 12;
    int direccion_bloque_datos;
    int cantidad_a_leer_en_bloque;
    FILE* fp = fopen(ruta_bin, "rb+");
    while (sum_bytes_leidos < bytes_leer) {
      printf("entrando a while\n");
      fseek(fp, direccion_bloque_actual, SEEK_SET);
      fread(&direccion_bloque_datos, sizeof(int), 1, fp);
      printf("direccion exacta de datos a leer: %i\n", direccion_bloque_datos);
      fseek(fp, direccion_bloque_datos, SEEK_SET);
      if (datos_restantes < 1024) {
        cantidad_a_leer_en_bloque = MIN(bytes_leer, datos_restantes);
        printf("cantidad_a_leer_en_bloque: %i\n", cantidad_a_leer_en_bloque);
        fread(buffer + sum_bytes_leidos, 1, cantidad_a_leer_en_bloque, fp);
        sum_bytes_leidos += cantidad_a_leer_en_bloque;
        //printf("buffer actual: %s\n", buffer); //errores
      }
      else{
        cantidad_a_leer_en_bloque = MIN(bytes_leer, 1024);
        printf("cantidad_a_leer_en_bloque: %i\n", cantidad_a_leer_en_bloque);
        fread(buffer + sum_bytes_leidos, 1, cantidad_a_leer_en_bloque, fp);
        sum_bytes_leidos += cantidad_a_leer_en_bloque;
        datos_restantes -= cantidad_a_leer_en_bloque;
        direccion_bloque_actual += 4;
        //printf("buffer actual: %s\n", buffer); //errores
      }
      if (direccion_bloque_actual == 12 + (252*4) + file_desc->direccion_bloque && datos_restantes != 0) {  //posicion del puntero indirecto
        int dir_bloque_datos_indir = file_desc->next_bloque;
        direccion_bloque_actual = dir_bloque_datos_indir;
      }
    }

    fclose(fp);

    return sum_bytes_leidos;  // retornando los bytes que se escribieron
  }
  return -1;
}
