/*
 * main.c
 *
 * UDP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP client
 * portable across Windows, Linux and macOS.
 */

#if defined WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "protocol.h"


void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}


int main(int argc, char *argv[]) {
	// Parsing argomenti
	const char *server_address = "127.0.0.1";
	int port = SERVER_PORT;
	printf("Server: %s, porta: %d\n", server_address, port);
	const char *req_str = NULL;

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-s") == 0) {
			if (i + 1 < argc)
				server_address = argv[++i];
		} else if (strcmp(argv[i], "-p") == 0) {
			if (i + 1 < argc)
				port = atoi(argv[++i]);
		} else if (strcmp(argv[i], "-r") == 0) {
			if (i + 1 < argc)
				req_str = argv[++i];
		}
	}

	if (!req_str) {
		printf("In uso: %s [-s server] [-p port] -r \"type city\"\n", argv[0]);
		return -1;
	}

#if defined WIN32
	// Inizializzazione Winsock
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		printf("%s", "Errore in WSAStartup()\n");
		return -1;
	}
#endif

	// Creazione socket
	int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (my_socket < 0) {
		printf("%s", "Creazione della socket fallita.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	// Configurazione indirizzo server
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_address); // IP del server
	server_addr.sin_port = htons(SERVER_PORT); // Porta del server

	// Connessione al server
	if (connect(my_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		printf("%s", "Connessione fallita.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	// Serializzazione richiesta: 1 byte type + CITY_NAME_LEN bytes city
	char req_buf[1 + CITY_NAME_LEN];
	memset(req_buf, 0, sizeof(req_buf));
	// type: primo carattere non spazio
	char type = '\0';
	for (const char *p = req_str; *p; ++p) {
		if (!isspace((unsigned char)*p)) {
			type = *p;
			break;
		}
	}
	req_buf[0] = type;
	// city: stringa dopo primo spazio (se presente)
	const char *city_start = NULL;
	const char *p = req_str;
	while (*p && isspace((unsigned char)*p))
		++p;
	if (*p) {
		++p;
		if (*p == ' ')
			++p;
		city_start = p;
	}
	if (city_start)
		strncpy((char*)(req_buf + 1), city_start, CITY_NAME_LEN - 1);

	// Invio richiesta
	if (send_all(my_socket, req_buf, (int)sizeof(req_buf)) != (int)sizeof(req_buf)) {
		printf("%s", "Errore invio richiesta.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	// Ricezione risposta: 4 byte status, 1 byte type, 4 byte value
	char resp_buf[4 + 1 + 4];
	if (recv_all(my_socket, resp_buf, (int)sizeof(resp_buf)) != (int)sizeof(resp_buf)) {
		printf("%s", "Errore ricezione risposta.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	int net_status;
	memcpy(&net_status, resp_buf, 4);
	int status = ntohl(net_status);
	char resp_type = (char)resp_buf[4];
	int net_value;
	memcpy(&net_value, resp_buf + 5, 4);
	int value_host = ntohl(net_value);
	float value_f;
	memcpy(&value_f, &value_host, sizeof(float));

	char server_ip_str[INET_ADDRSTRLEN];
	strncpy(server_ip_str, inet_ntoa(server_addr.sin_addr), sizeof(server_ip_str)-1);
	server_ip_str[sizeof(server_ip_str)-1] = '\0';

	// Costruzione messaggio da stampare dopo il prefisso richiesto
	if (status == STATUS_OK) {
		// Capitalizzazione city per output: prima lettera maiuscola
		char city[CITY_NAME_LEN];
		strncpy(city, (const char*)(req_buf + 1), CITY_NAME_LEN - 1);
		city[CITY_NAME_LEN - 1] = '\0';
		if (city[0]) city[0] = (char)toupper((unsigned char)city[0]);

		if (resp_type == 't') {
			printf("Ricevuto risultato dal server IP %s. %s: Temperatura = %.1f °C\n", server_ip_str, city, value_f);
		} else if (resp_type == 'h') {
			printf("Ricevuto risultato dal server IP %s. %s: Umidità = %.1f%%\n", server_ip_str, city, value_f);
		} else if (resp_type == 'w') {
			printf("Ricevuto risultato dal server IP %s. %s: Vento = %.1f km/h\n", server_ip_str, city, value_f);
		} else if (resp_type == 'p') {
			printf("Ricevuto risultato dal server IP %s. %s: Pressione = %.1f hPa\n", server_ip_str, city, value_f);
		} else {
			printf("Ricevuto risultato dal server IP %s. Richiesta valore non valida.\n", server_ip_str);
		}
	} else if (status == STATUS_CITY_NOT_AVAILABLE) {
		printf("Ricevuto risultato dal server IP %s. Città non disponibile.\n", server_ip_str);
	} else if (status == STATUS_INVALID_REQUEST) {
		printf("Ricevuto risultato dal server IP %s. Richiesta non valida.\n", server_ip_str);
	} else {
		printf("Ricevuto risultato dal server IP %s. Errore\n", server_ip_str);
	}

	// Chiusura della socket
	closesocket(my_socket);

	clearwinsock();

	return 0;
}
