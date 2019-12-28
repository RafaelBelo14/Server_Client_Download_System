make:
	gcc -c -g server.c -Wall
	gcc -o server server.c -lsodium
	gcc -c -g client.c -Wall
	gcc -o cliente client.c -lsodium
	gcc -c -g proxy.c -Wall
	gcc -o ircproxy proxy.c
