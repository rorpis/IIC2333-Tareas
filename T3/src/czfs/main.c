#include "cz_API.h"
#include <time.h>

int main(int argc, char *argv[])
{
  if(argc != 2)
	{
		printf("Modo de uso: %s <disk_file>\n", argv[0]);
		return 1;
	}

  ruta_bin = argv[1];

  if (!(ruta_bin)) {
    printf("Archivo %s no existe.\n", ruta_bin);
    return 1;
  }

/*FILE* fp = fopen(ruta_bin, "rb+");

fseek(fp, 0, SEEK_SET);
fwrite("1", 1, 1, fp);
char a[4] = "hola";
fwrite(a, sizeof(char), 4, fp);
fclose(fp);*/
  cz_ls();

char* b = calloc(2, sizeof(char));
for(int k = 0; k < 256; k++) {
  FILE* fp = fopen(ruta_bin, "rb+");
  fseek(fp, 1023 , SEEK_SET);
  b[0] = k;
  fwrite(b, 1, 1, fp);
  fclose(fp);
  printf("posiciion; %i\n", bitmap_get_free());
}
free(b);


  return 0;
}
