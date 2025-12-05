/*
 * main.c
 *
 * UDP Server - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a UDP server
 * portable across Windows, Linux and macOS.
 */

#if defined WIN32
#include <winsock2.h>
typedef int socklen_t;
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
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "protocol.h"


void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}


int main(int argc, char *argv[]) {
#if defined WIN32
	// Inizializzazione Winsock
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		printf("%s", "Errore in WSAStartup()\n");
		return -1;
	}
#endif

	// Parsing argomenti
	int port = SERVER_PORT;
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
			port = atoi(argv[++i]);
		}
	}

	// Inizializzazione generatore di numeri pseudocasuali
	srand((unsigned)time(NULL));

	// Creazione socket
	int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket < 0) {
		printf("%s", "Creazione della socket fallita.\n");
		clearwinsock();
		return -1;
	}

	// Configurazione server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind
	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		printf("%s", "bind() fallito.\n");
		closesocket(server_socket);
		clearwinsock();
		return -1;
	}

	// Listen
	if (listen(server_socket, QUEUE_SIZE) < 0) {
		printf("%s", "listen() fallito.\n");
		closesocket(server_socket);
		clearwinsock();
		return -1;
	}

	// Loop di accettazione delle connessioni
	printf("Server in ascolto sulla porta %d...\n", port);
	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
		if (client_socket < 0) {
			printf("%s", "accept() fallito.\n");
			continue;
		}

		char *client_ip = inet_ntoa(client_addr.sin_addr);

		// Ricezione richiesta: 1 byte type + CITY_NAME_LEN bytes
		char req_buf[1 + CITY_NAME_LEN];
		if (recv_all(client_socket, req_buf, (int)sizeof(req_buf)) != (int)sizeof(req_buf)) {
			closesocket(client_socket);
			continue;
		}

		char req_type = (char)req_buf[0];
		char city[CITY_NAME_LEN];
		strncpy(city, (const char*)(req_buf + 1), CITY_NAME_LEN - 1);
		city[CITY_NAME_LEN - 1] = '\0';

		// Log richiesta
		printf("Richiesta '%c %s' dal client IP %s\n", req_type ? req_type : '?', city, client_ip);

		// Validazione
		int status = STATUS_OK;
		char resp_type = req_type;
		float value = 0.0f;

		int type_valid = (req_type == 't' || req_type == 'h' || req_type == 'w' || req_type == 'p');
		if (!type_valid) {
			status = STATUS_INVALID_REQUEST;
			resp_type = '\0';
		} else if (!city_supported(city)) {
			status = STATUS_CITY_NOT_AVAILABLE;
			resp_type = '\0';
		} else {
			// Generazione valore
			if (req_type == 't') value = get_temperature();
			else if (req_type == 'h') value = get_humidity();
			else if (req_type == 'w') value = get_wind();
			else if (req_type == 'p') value = get_pressure();
		}

		// Preparazione buffer di risposta: status (4 bytes), type (1 byte), value (4 bytes)
		char resp_buf[4 + 1 + 4];
		int net_status = htonl(status);
		memcpy(resp_buf, &net_status, 4);
		resp_buf[4] = (char)resp_type;
		int value_bits = 0;
		if (status == STATUS_OK) {
			int tmp;
			memcpy(&tmp, &value, sizeof(float));
			value_bits = htonl(tmp);
		} else {
			int tmp = 0;
			value_bits = htonl(tmp);
		}
		memcpy(resp_buf + 5, &value_bits, 4);

		// Invio risposta
		if (send_all(client_socket, resp_buf, (int)sizeof(resp_buf)) != (int)sizeof(resp_buf)) {
			printf("%s", "send_all() fallito.\n");
		}

		//Chiusura socket client
		closesocket(client_socket);
	}

	//Chiusura server socket
	closesocket(server_socket);

	clearwinsock();
	return 0;
}
