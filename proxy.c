#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>


#define BUF_SIZE 4096

typedef struct save{
  int state;
  char *nome_ficheiro;
}Save;

//Variaveis globais
int shm_save;
Save *salvar;
pid_t childpid;

//Funçoes
void controlo(int argc, char **argv);
void cleanup(int sig);
void cria_mem();
unsigned int transferencia(int from, int to);
void handle(int client, const char *remote_host, const char *remote_port);

//Codigo das funçoes
int main(int argc, char **argv){
    int sock;
    struct addrinfo info, *endereco;
    int reuseaddr = 1; /* True */
    const char *local_host, *local_port, *remote_host, *remote_port;

    system("clear");

    if (argc != 3) {
        fprintf(stderr, "ircproxy <client port> <server port>\n");
        return 1;
    }

    signal(SIGINT, cleanup);

    local_host = remote_host = "127.0.0.1";
    local_port = argv[1];
    remote_port = argv[2];

    cria_mem();

    memset(&info, 0, sizeof info);
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(local_host, local_port, &info, &endereco) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    sock = socket(endereco->ai_family, endereco->ai_socktype, endereco->ai_protocol);
    if (sock == -1) {
        perror("socket");
        freeaddrinfo(endereco);
        return 1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
        perror("setsockopt");
        freeaddrinfo(endereco);
        return 1;
    }

    if (bind(sock, endereco->ai_addr, endereco->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(endereco);
        return 1;
    }

    if (listen(sock, 10) == -1) {
        perror("listen");
        freeaddrinfo(endereco);
        return 1;
    }

    freeaddrinfo(endereco);

    /* Main loop */
    while (1) {
        socklen_t size = sizeof(struct sockaddr_in);
        struct sockaddr_in their_addr;
        int newsock = accept(sock, (struct sockaddr*)&their_addr, &size);

        if (newsock == -1) {
            perror("accept");
        }
        else {
            if ((childpid = fork()) == 0){
              handle(newsock, remote_host, remote_port);
            }
            else if (childpid > 0){
              controlo(argc, argv);
            }
            else{
              printf("Erro no fork()\n");
            }
        }
    }

    close(sock);

    return 0;
}

unsigned int transferencia(int from, int to){

    char buf[BUF_SIZE];
    FILE *f;
    f = fopen("relatório_downloads", "a");
    unsigned int state = 0;
    size_t bytes_read, bytes_written;
    bytes_read = read(from, buf, BUF_SIZE);
    if (from > to){
      printf("Received from server: %s\n", buf);
    }
    else{
      printf("Received from client: %s\n", buf);
      if (strcmp(buf, "DOWNLOAD exemplo.txt NOR") == 0){
        if (salvar->state == 1){
          salvar->nome_ficheiro = "exemplo.txt";
          fprintf(f, "Downloaded: %s\n", salvar->nome_ficheiro);
        }
      }
      else if (strcmp(buf, "DOWNLOAD exemplo.jpg NOR") == 0){
        if (salvar->state == 1){
          salvar->nome_ficheiro = "exemplo.jpg";
          fprintf(f, "Downloaded: %s\n", salvar->nome_ficheiro);
        }
      }
      else if (strcmp(buf, "DOWNLOAD exemplo.wav NOR") == 0){
        if (salvar->state == 1){
          salvar->nome_ficheiro = "exemplo.wav";
          fprintf(f, "Downloaded: %s\n", salvar->nome_ficheiro);
        }
      }
    }
    if (bytes_read == 0) {
      state = 1;
    }
    else {
      bytes_written = write(to, buf, bytes_read);
      if (bytes_written == -1){
        state = 1;
      }
    }
    fclose(f);
    return state;
}

void handle(int client, const char *remote_host, const char *remote_port){
    struct addrinfo info, *endereco;
    int server = -1;
    unsigned int state = 0;
    fd_set set;
    unsigned int max_sock;

    memset(&info, 0, sizeof info);
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;

    /*getaddrinfo () converte sequências de texto legíveis que
    representam nomes de host ou endereços IP numa lista alocada
    dinamicamente de estruturas 'addrinfo'*/
    if (getaddrinfo(remote_host, remote_port, &info, &endereco) != 0) {
        printf("Erro no getaddinfo()\n");
        close(client);
        return;
    }

    server = socket(endereco->ai_family, endereco->ai_socktype, endereco->ai_protocol);
    if (server == -1) {
        printf("Erro no socket\n");
        close(client);
        return;
    }

    if (connect(server, endereco->ai_addr, endereco->ai_addrlen) == -1) {
        printf("Erro no connect\n");
        close(client);
        return;
    }

    if (client > server) {
        max_sock = client;
    }
    else {
        max_sock = server;
    }

    while (!state) {
        FD_ZERO(&set);
        FD_SET(client, &set);
        FD_SET(server, &set);
        if (select(max_sock + 1, &set, NULL, NULL, NULL) == -1) {
            printf("Erro no select\n");
            break;
        }
        if (FD_ISSET(client, &set)) {
            state = transferencia(client, server);
        }
        if (FD_ISSET(server, &set)) {
            state = transferencia(server, client);
        }
    }
    close(server);
    close(client);
}

void cria_mem(){

  //CRIA A MEMÓRIA PARTILHADA USANDO AS FUNÇÕES SHMGET E SHMAT
  if ((shm_save = shmget(IPC_PRIVATE, sizeof(Save), IPC_CREAT | 0777)) < 0){
    printf("Erro no shmget!\n");
  }
  else if((salvar = (Save *) shmat(shm_save, NULL, 0)) < 0) {
    printf("Erro no shmat!\n");
  }
}

void controlo(int argc, char **argv){

  char comando[BUF_SIZE];


  while(1){

    scanf("%s", comando);

    if (strcmp(comando, "SHOW") == 0){
      printf("\tInformação sobre a ligação:\n\n");
      printf("\tendereço de origem:\t127.0.0.1\n");
      printf("\tPorto de origem:\t%s\n", argv[1]);
      printf("\tEndereço de destino:\t127.0.0.1\n");
      printf("\tPorto de destino:\t%s\n\n", argv[2]);
    }
    else if(strcmp(comando, "SAVE") == 0){
      if (salvar->state == 0){
        salvar->state = 1;
      }
      else{
        salvar->state = 0;
      }
    }
    else{
      printf("\t[Wrong command]\n");
    }
  }
}

void cleanup(int sig){

    shmdt(&salvar);
    shmctl(shm_save, IPC_RMID, NULL);
    printf("\nShutting down proxy...\n");
    exit(0);

}
