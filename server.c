// Server program
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <signal.h>
#include <sodium.h>

#define MAXLINE 1024
#define CHUNK_SIZE 4096

typedef struct cliente{
  int i;
}Cliente;

//VARIÁVEIS

int listenfd, connfd, udpfd, nready, maxfdp1;
char buffer[MAXLINE];
char linha[MAXLINE], scanner[MAXLINE], *aux, type[MAXLINE], enc[MAXLINE];
pid_t childpid;
fd_set rset;
ssize_t n;
int i = 0;
int nread = 0;
socklen_t len;
struct sockaddr_in cliaddr, servaddr;
char mensagem[MAXLINE];
int shm_client;
Cliente *client;
unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];

char *lista[3] = {"exemplo.jpg", "exemplo.wav", "exemplo.txt"};


//FUNÇÕES

void lista_ficheiros();
void quit();
int max(int x, int y);
void tcp_processo(int connfd);
void download_img(int connfd);
void lista_ficheiros();
void download_txt(int connfd, char *enc);
void download_wav(int connfd);
void cria_mem();
void cleanup(int sig);
int encrypt(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);



//CÓDIGO DAS FUNÇÕES

int main(int argc, char *argv[]){

  int max_clientes = atoi(argv[2]);

  system("clear");

  if (argc != 3){
    printf("server <server port> <maximum number of clients>\n");
    exit(0);
  }

  if (sodium_init() != 0) {
    return 1;
  }


  /* create a shared memory */
  cria_mem();

  signal(SIGINT, cleanup);

  /* create listening TCP socket */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons((short) atoi(argv[1]));

  // binding server addr structure to listenfd
  bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  listen(listenfd, 5);

  /* create UDP socket */
  udpfd = socket(AF_INET, SOCK_DGRAM, 0);
  // binding server addr structure to udp sockfd
  bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

  while (i < max_clientes) {
    //wait for new connection
    connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
    if (connfd > 0) {
      client->i++;
      if ((childpid = fork()) == 0) {
        tcp_processo(connfd);
      }
    }
  }
  printf("\n\t[Excedeu o número máximo de clientes!]\n");
  printf("\n\t[Closing server...]\n");
  exit(0);

  return 0;
}

void tcp_processo(int connfd){

  char message[MAXLINE];
  sprintf(message, "Hello client number %d", client->i);

  nread = read(connfd, buffer, sizeof(buffer));
  buffer[nread] = '\0';
  write(connfd, (const char*)message, sizeof(buffer));

  while(1){

    read(connfd, buffer, sizeof(buffer));
    strcpy(linha, buffer);
    strcpy(scanner, buffer);
    aux = strtok(linha, " ");
    if (strcmp(aux, "LIST") == 0){
      lista_ficheiros();
    }

    else if (strcmp(aux, "DOWNLOAD") == 0){
      sscanf(scanner, "DOWNLOAD %s %s", type, enc);
      if (strcmp(type, "exemplo.jpg") == 0){
        download_img(connfd);
      }
      else if (strcmp(type, "exemplo.txt") == 0){
        download_txt(connfd, enc);
      }
      else if (strcmp(type, "exemplo.wav") == 0){
        download_wav(connfd);
      }
      else{
        printf("\t[Wrong command]\n");
      }
    }

    else if (strcmp(aux, "QUIT") == 0){
      quit();
      exit(0);
    }

    else{
      printf("\t[Wrong command]\n");
    }

  }

}

int max(int x, int y){
    if (x > y)
        return x;
    else
        return y;
}

void lista_ficheiros(){
  printf("\n\t-> [Entrou em server_files]\n");
  for (int id = 0; id < 3; id++){
    printf("\t%s\n", lista[id]);
  }
  sprintf(mensagem, "\n\t%s\n\t%s\n\t%s\n", lista[0], lista[1], lista[2]);
  write(connfd, mensagem, sizeof(buffer));
}

void quit(){
  client->i--;
  printf("\n\t-> [Closing connection with client...Clients connected: %d]\n\n\n", client->i);
  exit(0);
}

void download_txt(int connfd, char *enc){

  FILE *f;

  int words = 1;
  char c;
  char ch = '\0';

  if (strcmp(enc, "ENC") == 0){
    crypto_secretstream_xchacha20poly1305_keygen(key);
    if (encrypt("proxy_files/exemplo_encrypted.txt", "server_files/exemplo.txt", key) != 0) {
        printf("Nao deu!\n");
    }
    write(connfd, key, sizeof(key));
  }
  else if (strcmp(enc, "NOR") == 0){
    f = fopen("server_files/exemplo.txt","r");
    while((c=getc(f))!=EOF){
      fscanf(f , "%s" , buffer);
      if(isspace(c)|| c == '\t')
      words++;
    }


    write(connfd, &words, sizeof(words));
    rewind(f);

    while(ch != EOF){
      fscanf(f , "%s" , buffer);
      write(connfd, buffer, 10);
      ch = fgetc(f);
    }
    printf("The file was sent successfully");
  }
  else{
    printf("\t[Wrong command]\n");
  }

}

void download_img(int connfd){

  if (strcmp(enc, "ENC") == 0){
    crypto_secretstream_xchacha20poly1305_keygen(key);
    if (encrypt("proxy_files/exemplo_encrypted.jpg", "server_files/exemplo.jpg", key) != 0) {
        printf("Nao deu!\n");
    }
    write(connfd, key, sizeof(key));
  }
  else if (strcmp(enc, "NOR") == 0){
    char* tcp_message = "\tDownload Started...\n";
    write(connfd, tcp_message, sizeof(buffer));

    //ENVIAR FOTOS
    //Get Picture Size
    printf("\n\tGetting Picture Size...\n");
    FILE *picture;
    picture = fopen("server_files/exemplo.jpg", "r");
    int size;
    fseek(picture, 0, SEEK_END);
    size = ftell(picture);
    fseek(picture, 0, SEEK_SET);

    //Send Picture Size
    printf("\tSending Picture Size...\n");
    write(connfd, &size, sizeof(size));

    //Send Picture as Byte Array (without need of a buffer as large as the image file)
    printf("\tSending Picture as Byte Array...\n");
    char send_buffer[size]; // no link between BUFSIZE and the file size
    int nb = fread(send_buffer, 1, sizeof(send_buffer), picture);
    while(!feof(picture)) {
        write(connfd, send_buffer, nb);
        nb = fread(send_buffer, 1, sizeof(send_buffer), picture);
    }
    printf("\tDone!\n");
    fclose(picture);
  }
  else{
    printf("\t[Wrong command]\n");
  }

}

void download_wav(int connfd){

  if (strcmp(enc, "ENC") == 0){
    crypto_secretstream_xchacha20poly1305_keygen(key);
    if (encrypt("proxy_files/exemplo_encrypted.wav", "server_files/exemplo.wav", key) != 0) {
        printf("Nao deu!\n");
    }
    write(connfd, key, sizeof(key));
  }
  else if (strcmp(enc, "NOR") == 0){

    char* tcp_message = "\tDownload Started...\n";
    write(connfd, tcp_message, sizeof(buffer));


    //ENVIAR SOM
    //Get Picture Size
    printf("\n\tGetting Sound Size...\n");
    FILE *sound;
    sound = fopen("server_files/exemplo.wav", "r");
    int size;
    fseek(sound, 0, SEEK_END);
    size = ftell(sound);
    fseek(sound, 0, SEEK_SET);

    //Send Picture Size
    printf("\tSending Sound Size...\n");
    write(connfd, &size, sizeof(size));

    //Send Picture as Byte Array (without need of a buffer as large as the image file)
    printf("\tSending Sound as Byte Array...\n");
    char send_buffer[size]; // no link between BUFSIZE and the file size
    int nb = fread(send_buffer, 1, sizeof(send_buffer), sound);
    while(!feof(sound)){
        write(connfd, send_buffer, nb);
        nb = fread(send_buffer, 1, sizeof(send_buffer), sound);
    }
    write(connfd, send_buffer, nb);
    printf("\tDone!\n");
    fclose(sound);
  }
  else{
    printf("\t[Wrong command]\n");
  }

}

void cria_mem(){

  //CRIA A MEMÓRIA PARTILHADA USANDO AS FUNÇÕES SHMGET E SHMAT
  if ((shm_client = shmget(IPC_PRIVATE, sizeof(Cliente), IPC_CREAT | 0777)) < 0){
    printf("Erro no shmget!\n");
  }
  else if((client = (Cliente *) shmat(shm_client, NULL, 0)) < 0) {
    printf("Erro no shmat!\n");
  }
}

void cleanup(int sig){

    printf("\n\tCleaning memories...\n");
    //liberar memoria partilhada
    shmdt(&client);
    shmctl(shm_client, IPC_RMID, NULL);
    exit(0);
}


int encrypt(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE];
    unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    crypto_secretstream_xchacha20poly1305_init_push(&st, header, key);
    fwrite(header, 1, sizeof header, fp_t);
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
        crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
                                                   NULL, 0, tag);
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);
    fclose(fp_t);
    fclose(fp_s);
    return 0;
}
/*void processo_udp(int udpfd){

  char* udp_message = "\t\tWelcome to the server by UDP!\n\n";

  len = sizeof(cliaddr);
  bzero(buffer, sizeof(buffer));
  n = recvfrom(udpfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
  puts(buffer);
  sendto(udpfd, (const char*)udp_message, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
  exit(0);
}*/
