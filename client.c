/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 **********************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sodium.h>

#define BUF_SIZE  2000
#define CHUNK_SIZE 4096
struct stat st = {0};

//VARIÁVEIS

int fd;
int state = 1;
int nread = 0;
int n_cliente;
char diretoria[BUF_SIZE];
char buffer[BUF_SIZE];
char endServer[100];
struct sockaddr_in addr;
struct hostent *hostPtr;
char linha[BUF_SIZE], scanner[BUF_SIZE], type[BUF_SIZE], enc[BUF_SIZE], *aux;
char *message = "Hello server";
char mensagem[BUF_SIZE];
unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];


//FUNÇÕES
void erro(char *msg);
//void client_udp(int argc, char *argv[]);
void client_tcp(char *mensagem);
void download_img(int fd, char *enc);
void download_txt(int fd, char *enc);
void download_wav(int fd, char *enc);
int decrypt(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES]);


//CÓDIGO DAS FUNÇÕES

int main(int argc, char *argv[]){

  system("clear");

  //O CLIENT SÓ PODE RECEBER 3 ARGUMENTOS
  if (argc != 3){
    printf("cliente <proxy address> <client port>\n");
    exit(0);
  }

  if (sodium_init() != 0) {
    return 1;
  }

  //CONFIGURAÇÃO DA LIGAÇÃO TCP
  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0){
    perror("Nao consegui obter endereço");
  }
  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  //CRIAÇÃO DO SOCKET
  if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
    perror("socket");
  }

  if( connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0){
    perror("Connect");
  }

  write(fd, message, 1 + strlen(message));
  read(fd, buffer, sizeof(buffer));
  sscanf(buffer, "Hello client number %d", &n_cliente);
  bzero(buffer, BUF_SIZE);

  struct stat st = {0};

  snprintf(diretoria, BUF_SIZE, "%s%d", "downloads", n_cliente);

  if (stat(diretoria, &st) == -1) {
    mkdir(diretoria , 0700);
  }

  bzero(buffer, BUF_SIZE);

  //O CLIENTE ENVIA OS COMANDOS QUE QUER ATÉ AO COMANDO 'QUIT'
  while(state){

    printf("\n\tNEW COMMAND: ");
    gets(mensagem);
    write(fd, mensagem, 1 + strlen(mensagem));
    strcpy(linha, mensagem);
    strcpy(scanner, mensagem);
    aux = strtok(linha, " ");

    if (strcmp(aux, "LIST") == 0){
      read(fd, buffer, sizeof(buffer));
      printf("\n\tLista de ficheiros:\n%s", buffer);
    }
    else if (strcmp(aux, "DOWNLOAD") == 0){
      sscanf(scanner, "DOWNLOAD %s %s", type, enc);
      if (strcmp(type, "exemplo.jpg") == 0){
        download_img(fd, enc);
      }
      else if (strcmp(type, "exemplo.txt") == 0){
        download_txt(fd, enc);
      }
      else if (strcmp(type, "exemplo.wav") == 0){
        download_wav(fd, enc);
      }
      else{
        printf("\t[Wrong command]\n");
      }
    }
    else if (strcmp(aux, "QUIT") == 0){
      state = 0;
      /* removes de directory if its empty */
      rmdir(diretoria);
    }
    else{
      printf("\t[Wrong command]\n");
    }

  }
  close(fd);
  exit(0);

}

void download_txt(int fd, char *enc){

  FILE *fp = fopen(strcat(diretoria, "/exemplo_download.txt"), "w");
  int ch = 0;
  int words;
  size_t bytes_read;
  clock_t start, stop;
  start = clock();

  if (strcmp(enc, "ENC") == 0){
    read(fd, key, sizeof(key));
    if (decrypt(diretoria, "proxy_files/exemplo_encrypted.txt", key) != 0) {
        printf("Nao deu!\n");
    }

    stop = clock();
    printf("\n\tNome do ficheiro transferido:\texemplo.txt\n");
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms\n\n", stop-start );

  }
  else if (strcmp(enc, "NOR") == 0){
    read(fd, &words, sizeof(int));

    while(ch != words){
      bytes_read += read(fd, buffer, 10);
      fprintf(fp , "%s " , buffer);
      ch++;
    }
    bytes_read = bytes_read / 2;

    stop = clock();
    printf("\n\tNome do ficheiro transferido:\texemplo.txt\n");
    printf("\tTotal de bytes recebidos:\t%ld bytes\n", bytes_read);
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms\n\n", stop-start );

  }
  else{
    printf("\t[Wrong command]\n");
  }

  fclose(fp);

}

void download_img(int fd, char *enc){

  int size;
  size_t bytes_read;
  int rec_size = 0;
  clock_t start, stop;
  start = clock();

  if (strcmp(enc, "ENC") == 0){
    read(fd, key, sizeof(key));
    if (decrypt(strcat(diretoria, "/exemplo_download.jpg"), "proxy_files/exemplo_encrypted.jpg", key) != 0) {
        printf("Nao deu!\n");
    }

    stop = clock();
    printf("\n\tNome do ficheiro transferido:\texemplo.jpg\n");
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms\n\n", stop-start );

  }
  else if (strcmp(enc, "NOR") == 0){

    //Read Picture Size
    read(fd, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    printf("\tReading Image Size...\n");

    read(fd, &size, sizeof(int));

    //Read Picture Byte Array and Copy in file
    printf("\tReading Image Byte Array\n");
    char p_array[size];
    FILE *image = fopen(strcat(diretoria, "/exemplo_download.jpg"), "w");
    int nb = read(fd, p_array, sizeof(p_array));
    bytes_read = size;
    rec_size += nb;
    while(rec_size < size){
      fwrite(p_array, 1, nb, image);
      nb = read(fd, p_array, sizeof(p_array));
      rec_size += nb;
    }


    fclose(image);
    stop = clock();

    printf("\n\tNome do ficheiro transferido:\texemplo.jpg\n");
    printf("\tTotal de bytes recebidos:\t%ld KB\n", bytes_read/1024);
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms\n\n", stop-start);

  }
  else{
    printf("\t[Wrong command]\n");
  }

}

void download_wav(int fd, char *enc){

  int size;
  int rec_size = 0;
  size_t bytes_read;
  clock_t start, stop;

  start = clock();
  if (strcmp(enc, "ENC") == 0){
    read(fd, key, sizeof(key));
    if (decrypt(strcat(diretoria, "/exemplo_download.wav"), "proxy_files/exemplo_encrypted.wav", key) != 0) {
        printf("Nao deu!\n");
    }

    stop = clock();
    printf("\n\tNome do ficheiro transferido:\texemplo.wav\n");
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms\n\n", stop-start );

  }
  else if (strcmp(enc, "NOR") == 0){

    //Read Picture Size
    read(fd, buffer, sizeof(buffer));
    printf("%s\n", buffer);
    printf("\tReading Sound Size...\n");

    read(fd, &size, sizeof(int));

    //Read Picture Byte Array and Copy in file
    printf("\tReading Sound Byte Array\n");
    char p_array[size];
    FILE *sound = fopen(strcat(diretoria, "/exemplo_download.wav"), "w");
    int nb = read(fd, p_array, size);
    bytes_read = size;
    rec_size += nb;

    while(rec_size < size){
      fwrite(p_array, 1, nb, sound);
      nb = read(fd, p_array, size);
      rec_size += nb;
    }

    fclose(sound);
    stop = clock();

    printf("\n\tNome do ficheiro transferido:\texemplo.wav\n");
    printf("\tTotal de bytes recebidos:\t%ld KB\n", bytes_read/1024);
    printf("\tProtocolo de transporte utilizado na transferência do ficheiro:\tTCP\n");
    printf("\tTempo total para download do ficheiro: %.3ld ms", stop-start);

  }
  else{
    printf("\n[Wrong command]\n");
  }

}

void erro(char *msg) {
	printf("Erro: %s\n", msg);
	exit(-1);
}

int decrypt(const char *target_file, const char *source_file, const unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES])
{
    unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char  buf_out[CHUNK_SIZE];
    unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state st;
    FILE          *fp_t, *fp_s;
    unsigned long long out_len;
    size_t         rlen;
    int            eof;
    int            ret = -1;
    unsigned char  tag;

    fp_s = fopen(source_file, "rb");
    fp_t = fopen(target_file, "wb");
    fread(header, 1, sizeof header, fp_s);
    if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, key) != 0) {
        goto ret; /* incomplete header */
    }
    do {
        rlen = fread(buf_in, 1, sizeof buf_in, fp_s);
        eof = feof(fp_s);
        if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag, buf_in, rlen, NULL, 0) != 0) {
            goto ret; /* corrupted chunk */
        }
        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && ! eof) {
            goto ret; /* premature end (end of file reached before the end of the stream) */
        }
        fwrite(buf_out, 1, (size_t) out_len, fp_t);
    } while (! eof);

    ret = 0;
ret:
    fclose(fp_t);
    fclose(fp_s);
    return ret;
}

/*void client_udp(int argc, char *argv[]){

  struct sockaddr_in addr;
  int s,recv_len;
  int slen = sizeof(addr);
  char buf[BUF_SIZE];
  char endServer[100];
  struct hostent *hostPtr;
  char *mensagem = "Hello server";

  system("clear");

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
      erro("Nao consegui obter endereço");
  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  // Cria um socket para recepção de pacotes UDP
  if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    erro("Erro na criação do socket");
  }

  sendto(s, mensagem, strlen(mensagem), 0,(struct sockaddr *)&addr, sizeof(addr));

  if((recv_len = recvfrom(s, buf,BUF_SIZE,0,(struct sockaddr *)&addr,(socklen_t *)&slen)) == -1)
  {
  perror("Erro no recvfrom");
  }

  close(s);
  exit(0);
}*/
