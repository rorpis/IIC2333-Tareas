#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mensajes.h"
#include "funciones.h"
#include "cartas.h"

int main(int argc, char *argv[]) {
  if(argc != 5) {
    printf("Modo de uso: %s [-i <ip_address>] [-p <tcp-port>]\n", argv[0]);
    return 1;
  }

  char* server_address = NULL;
  int port = -1;
  for(int i = 0; i < argc; i++) {
    if(strcmp(argv[i], "-i") == 0)
      server_address = argv[i + 1];
    if(strcmp(argv[i], "-p") == 0)
      port = atoi(argv[i + 1]);
  }

  if(server_address == NULL || port == -1) {  // si nos entregaron los parametros incorrectos
    printf("Modo de uso: %s [-i <ip_address>] [-p <tcp-port>]\n", argv[0]);
    return 1;
  }

  printf("Address %s, Puerto: %i\n", server_address, port);

  int server_fd;  // socket del server
  int opt = 1;  // ni idea q es esto
  struct sockaddr_in address;  // info de conexion
  int addrlen = sizeof(address);

  if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  //address.sin_addr.s_addr = INADDR_ANY;
  address.sin_addr.s_addr = inet_addr(server_address);
  address.sin_port = htons(port);

  if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if(listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  char buffer[2057];  // 8 + 8 + 8*255 + 1
  char* mensaje_enviar;
  char** mensaje_recibir;  // [mensaje_id, tamano, mensaje]

  int* clientes = calloc(2, sizeof(int));
  char** nicknames = calloc(2, sizeof(char*));
  for(int i = 0; i < 2; i++) {
    if(clientes[i] == 0) {
      if((clientes[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      printf("Aceptando conexion cliente %i. Esperando confirmacion\n", i);
      read(clientes[i], buffer, 2057);
      mensaje_recibir = decodificar(buffer);

      if(atoi(mensaje_recibir[0]) == start_conection) {  // es decir, si recibo id de start conection
        // Enviar mensaje de conection established
        printf("Recibo confirmacion. Envio respuesta\n");
        mensaje_enviar = codificar(conection_established, "");
        send(clientes[i], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
      } else {
        // quizas estos errores hay que manejarlos diferente
        perror("no se recibio mensaje de conexion establecida");
        exit(EXIT_FAILURE);
      }
      free_decodificacion(mensaje_recibir);

      printf("Solicito nickname\n");
      mensaje_enviar = codificar(ask_nickname, "");
      send(clientes[i], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);

      read(clientes[i], buffer, 2057);
      mensaje_recibir = decodificar(buffer);
      if(atoi(mensaje_recibir[0]) != return_nickname) {  // es decir, si no recibo id de return nickname
        // quizas estos errores hay que manejarlos diferente
        perror("no se recibio mensaje de retornar nickname");
        exit(EXIT_FAILURE);
      }
      nicknames[i] = calloc(256, sizeof(char));
      strcpy(nicknames[i], mensaje_recibir[2]);
      free_decodificacion(mensaje_recibir);
    }
  }
  printf("tengo nickname %s y %s\n", nicknames[0], nicknames[1]);

  /* Envio oponentes */
  mensaje_enviar = codificar(opponent_found, nicknames[1]);
  send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  mensaje_enviar = codificar(opponent_found, nicknames[0]);
  send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  sleep(1);  // si no espero se acoplan mensajes

  int pot[2] = {1000, 1000};
  //pot[0] = 11;
  printf("%i %i\n", pot[0], pot[1]);
  /* Envio pots */
  char* pot_string = malloc(sizeof(char*));
  itoa(pot[0], pot_string, 10);
  mensaje_enviar = codificar(initial_pot, pot_string);
  send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  itoa(pot[1], pot_string, 10);
  mensaje_enviar = codificar(initial_pot, pot_string);
  send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  //free(pot_string);

  sleep(1);

  mensaje_enviar = codificar(game_start, "");
  send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  mensaje_enviar = codificar(game_start, "");
  send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);

  sleep(1);

  int jugando = 1;
  int ganador = -1;
  while(jugando == 1) {
    if (pot[0] < 10) {
      ganador = 1;
      jugando = 0;
    }
    else if (pot[1] < 10) {
      ganador = 0;
      jugando = 0;
    }
    else {
      Mazo* mazo = crear_mazo();

      pot[0] -= 10;
      pot[1] -= 10; //por mientras para evitar loops infinitos

      itoa(pot[0], pot_string, 10);
      mensaje_enviar = codificar(start_round, pot_string);
      send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      itoa(pot[1], pot_string, 10);
      mensaje_enviar = codificar(start_round, pot_string);
      send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      //free(pot_string);
      sleep(1);

      printf("pot string\n");
      itoa(10, pot_string, 10);
      printf("%s\n", pot_string);
      mensaje_enviar = codificar(initial_bet, pot_string);
      send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      //free(pot_string);
      printf("pot string\n");
      itoa(10, pot_string, 10);
      printf("%s\n", pot_string);
      mensaje_enviar = codificar(initial_bet, pot_string);
      send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      //free(pot_string);
      sleep(1);
      // se juega la ronda

      /* 5 cartas */
      int** cartas_j1 = malloc(5 * sizeof(int*));
      printf("cartas j1: \n");
      for(int k = 0; k < 5; k++) {
        cartas_j1[k] = sacar_carta(mazo);
        printf("%i %i\n", cartas_j1[k][0], cartas_j1[k][1]);
      }
      mensaje_enviar = codificar_cartas(five_cards, cartas_j1, 5);

      send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      int** cartas_j2 = malloc(5 * sizeof(int*));
      printf("cartas j2: \n");
      for(int k = 0; k < 5; k++) {
        cartas_j2[k] = sacar_carta(mazo);
        printf("%i %i\n", cartas_j2[k][0], cartas_j2[k][1]);
      }
      mensaje_enviar = codificar_cartas(five_cards, cartas_j2, 5);

      send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
      free_codificacion(mensaje_enviar);
      sleep(1);

      srand(time(NULL));
      int primer_jugador = rand() % 2;
      primer_jugador = rand() % 2;
      primer_jugador = 0;
      if (primer_jugador == 0) {  //parte jugador 0 -------------
        printf("parte jugador 0\n");
        send(clientes[0], "000010110000000100000001", 24, 0);
        send(clientes[1], "000010110000000100000010", 24, 0);
        sleep(1);

        mensaje_enviar = codificar(get_cards_to_cange, "");
        send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        Decodificar_Mazo* mensaje_recibir_cartas;  // estructura

        read(clientes[0], buffer, 2057);
        mensaje_recibir_cartas = decodificar_cartas(buffer);
        if(atoi(mensaje_recibir_cartas->mensaje_id) != return_cards_to_change) {
          // quizas estos errores hay que manejarlos diferente
          perror("no se recibio cartas a cambiar");
          exit(EXIT_FAILURE);
        }
        printf("cartas a cambiar de jugador 0:\n");
        for(int k = 0; k < mensaje_recibir_cartas->cantidad_cartas; k++) {
          printf("[%i]: %i %i\n", k + 1, mensaje_recibir_cartas->cartas[k][0], mensaje_recibir_cartas->cartas[k][1]);
          for (int i = 0; i < 5; i++) {
            if (cartas_j1[i][0] == mensaje_recibir_cartas->cartas[k][0] &&
            cartas_j1[i][1] == mensaje_recibir_cartas->cartas[k][1]) {
              cartas_j1[i] = sacar_carta(mazo);
            }
          }
        }
        printf("cartas nuevas jugador 0: \n");
        for(int k = 0; k < 5; k++) {
          printf("[%i]: %i %i\n", k + 1, cartas_j1[k][0], cartas_j1[k][1]);
        }

        mensaje_enviar = codificar_cartas(five_cards, cartas_j1, 5);
        send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        // se cambia con el otro jugador
        mensaje_enviar = codificar(get_cards_to_cange, "");
        send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        read(clientes[1], buffer, 2057);
        mensaje_recibir_cartas = decodificar_cartas(buffer);
        if(atoi(mensaje_recibir_cartas->mensaje_id) != return_cards_to_change) {
          // quizas estos errores hay que manejarlos diferente
          perror("no se recibio cartas a cambiar");
          exit(EXIT_FAILURE);
        }
        printf("cartas a cambiar de jugador 1:\n");
        for(int k = 0; k < mensaje_recibir_cartas->cantidad_cartas; k++) {
          printf("[%i]: %i %i\n", k + 1, mensaje_recibir_cartas->cartas[k][0], mensaje_recibir_cartas->cartas[k][1]);
          for (int i = 0; i < 5; i++) {
            if (cartas_j2[i][0] == mensaje_recibir_cartas->cartas[k][0] &&
            cartas_j2[i][1] == mensaje_recibir_cartas->cartas[k][1]) {
              cartas_j2[i] = sacar_carta(mazo);
            }
          }
        }
        printf("cartas nuevas jugador 1: \n");
        for(int k = 0; k < 5; k++) {
          printf("[%i]: %i %i\n", k + 1, cartas_j2[k][0], cartas_j2[k][1]);
        }

        mensaje_enviar = codificar_cartas(five_cards, cartas_j2, 5);
        send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        int* bets = malloc(4*sizeof(int));
        bets[0] = 2;
        bets[1] = 3;
        bets[2] = 4;
        bets[3] = 5;
        mensaje_enviar = codificar_ints(get_bet, bets, 4);
        //printf("%s\n", mensaje_enviar);
        send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        read(clientes[0], buffer, 2057);
        printf("%s\n", buffer);
        mensaje_recibir = decodificar(buffer);
        if(atoi(mensaje_recibir[0]) != return_bet) {
          // quizas estos errores hay que manejarlos diferente
          perror("no se recibio mensaje de retornar bet");
          exit(EXIT_FAILURE);
        }
        int n_bet = mensaje_recibir[2][0];
        printf("bet recibido: %i\n", n_bet);
        int bet_error = 1;
        while (bet_error == 1) {
          for (int i = 0; i < 5; i++) {
            if (bets[i] == n_bet) {
              bet_error = 0;
            }
          }
          if (bet_error == 1) {
            mensaje_enviar = codificar(error_bet, "");
            send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
            free_codificacion(mensaje_enviar);
            sleep(1);

            mensaje_enviar = codificar_ints(get_bet, bets, 5);
            send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
            free_codificacion(mensaje_enviar);
            sleep(1);

            read(clientes[0], buffer, 2057);
            mensaje_recibir = decodificar(buffer);
            if(atoi(mensaje_recibir[0]) != return_bet) {
              // quizas estos errores hay que manejarlos diferente
              perror("no se recibio mensaje de retornar bet");
              exit(EXIT_FAILURE);
            }
            n_bet = mensaje_recibir[2][0];
          }
        }
        sleep(1);
        mensaje_enviar = codificar(ok_bet, "");
        send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        //bet 2

        int* bets2 = malloc(5*sizeof(int));
        int n_tot;
        if (n_bet == 2) {
          bets2[0] = 1;
          bets2[1] = 2;
          bets2[2] = 3;
          bets2[3] = 4;
          bets2[4] = 5;
          n_tot = 5;
        }
        if (n_bet == 3) {
          bets2[0] = 1;
          bets2[1] = 3;
          bets2[2] = 4;
          bets2[3] = 5;
          n_tot = 4;
        }
        if (n_bet == 4) {
          bets2[0] = 1;
          bets2[1] = 4;
          bets2[2] = 5;
          n_tot = 3;
        }
        if (n_bet == 5) {
          bets2[0] = 1;
          bets2[1] = 5;
          n_tot = 2;
        }
        mensaje_enviar = codificar_ints(get_bet, bets2, n_tot);
        send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        read(clientes[1], buffer, 2057);
        mensaje_recibir = decodificar(buffer);
        if(atoi(mensaje_recibir[0]) != return_bet) {
          // quizas estos errores hay que manejarlos diferente
          perror("no se recibio mensaje de retornar bet");
          exit(EXIT_FAILURE);
        }
        n_bet = mensaje_recibir[2][0];
        bet_error = 1;
        while (bet_error == 1) {
          for (int i = 0; i < n_tot; i++) {
            if (bets2[i] == n_bet) {
              bet_error = 0;
            }
          }
          if (bet_error == 1) {
            mensaje_enviar = codificar(error_bet, "");
            send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
            free_codificacion(mensaje_enviar);
            sleep(1);

            mensaje_enviar = codificar_ints(get_bet, bets2, n_tot);
            send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
            free_codificacion(mensaje_enviar);
            sleep(1);

            read(clientes[1], buffer, 2057);
            mensaje_recibir = decodificar(buffer);
            if(atoi(mensaje_recibir[0]) != return_bet) {
              // quizas estos errores hay que manejarlos diferente
              perror("no se recibio mensaje de retornar bet");
              exit(EXIT_FAILURE);
            }
            n_bet = mensaje_recibir[2][0];
          }
        }
        sleep(1);
        mensaje_enviar = codificar(ok_bet, "");
        send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
        free_codificacion(mensaje_enviar);
        sleep(1);

        printf("bet seteado en: %i\n", n_bet);

        break; //no tenemos el resto, forzamos salida

      }
    }
  }

  sleep(1);
  mensaje_enviar = codificar(game_end, "");
  send(clientes[0], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);
  mensaje_enviar = codificar(game_end, "");
  send(clientes[1], mensaje_enviar, strlen(mensaje_enviar), 0);
  free_codificacion(mensaje_enviar);

  sleep(1);
  //enviar imagenes

  return 0;
}
