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
#include <stdint.h>

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
		fprintf(stderr, "Errore in WSAStartup()\n");
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

	// Creazione socket UDP
	int server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server_socket < 0) {
		fprintf(stderr, "Creazione della socket UDP fallita.\n");
		clearwinsock();
		return -1;
	}

	// Configurazione server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Bind
	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
		fprintf(stderr, "bind() fallito.\n");
		closesocket(server_socket);
		clearwinsock();
		return -1;
	}

	printf("Server UDP in ascolto sulla porta %d...\n", port);

	while (1) {
		unsigned char req_buf[REQUEST_LEN];
		struct sockaddr_storage client_addr;
		socklen_t client_len = sizeof(client_addr);
		ssize_t recvd = recvfrom(server_socket, (char*)req_buf, (int)sizeof(req_buf), 0, (struct sockaddr*)&client_addr, &client_len);
		if (recvd != (ssize_t)sizeof(req_buf)) {
			// Si ignorano datagrammi malformati
			continue;
		}

		// Conversione dell'indirizzo del client in nome e IP per il logging
		char client_ip[INET_ADDRSTRLEN] = {0};
		char client_name[NI_MAXHOST];
		if (client_addr.ss_family == AF_INET) {
			struct sockaddr_in *c = (struct sockaddr_in*)&client_addr;
			inet_ntop(AF_INET, &c->sin_addr, client_ip, sizeof(client_ip));
			if (getnameinfo((struct sockaddr*)c, sizeof(struct sockaddr_in), client_name, sizeof(client_name), NULL, 0, 0) != 0) {
				strncpy(client_name, client_ip, sizeof(client_name)-1);
				client_name[sizeof(client_name)-1] = '\0';
			}
		} else {
			strncpy(client_ip, "?", sizeof(client_ip)-1);
			strncpy(client_name, "?", sizeof(client_name)-1);
		}

		char req_type = (char)req_buf[0];
		char city[CITY_NAME_LEN];
		strncpy(city, (const char*)(req_buf + 1), CITY_NAME_LEN - 1);
		city[CITY_NAME_LEN - 1] = '\0';

		// Standardizzazione per il logging
		char city_log[CITY_NAME_LEN];
		strncpy(city_log, city, CITY_NAME_LEN);
		if (city_log[0]) {
			city_log[0] = (char)toupper((unsigned char)city_log[0]);
			for (size_t i = 1; city_log[i]; ++i)
        		city_log[i] = (char)tolower((unsigned char)city_log[i]);
		}

		// Log richiesta
		printf("Richiesta ricevuta da %s (ip %s): type='%c', city='%s'\n", client_name, client_ip, req_type ? req_type : '?', city_log);

		// Validazione
		unsigned int status = STATUS_OK;
		char resp_type = req_type;
		float value = 0.0f;
        
		uint8_t type_valid = (req_type == 't' || req_type == 'h' || req_type == 'w' || req_type == 'p');
		// Buffer di risposta
		unsigned char resp_buf[RESPONSE_LEN];
		// Controllo di tab e/o caratteri non validi in city
		uint8_t not_allowed = 0;
		for (const char *p = city; *p; ++p) {
			if (*p == '\t') {
				not_allowed++;
				break;
			}
			if (!isalpha((unsigned char)*p) && *p != ' ') {
				not_allowed++;
				break;
			}
		}
		if (!type_valid || not_allowed) {
			status = STATUS_INVALID_REQUEST;
			resp_type = '\0';
		} else if (!city_supported(city)) {
			status = STATUS_CITY_NOT_AVAILABLE;
			resp_type = '\0';
		} else {
			if (req_type == 't')
				value = get_temperature();
			else if (req_type == 'h')
				value = get_humidity();
			else if (req_type == 'w')
				value = get_wind();
			else if (req_type == 'p')
				value = get_pressure();
		}
		uint32_t net_status = htonl((uint32_t)status);
		memcpy(resp_buf + RESP_STATUS_OFFSET, &net_status, sizeof(net_status));
		resp_buf[RESP_TYPE_OFFSET] = resp_type;
		uint32_t valbits = 0;
		if (status == STATUS_OK) {
			uint32_t tmp;
			memcpy(&tmp, &value, sizeof(value));
			valbits = htonl(tmp);
		} else {
			uint32_t tmp = 0;
			valbits = htonl(tmp);
		}
		memcpy(resp_buf + RESP_VALUE_OFFSET, &valbits, sizeof(valbits));

		// Invio risposta con sendto verso l'indirizzo del client ricevuto
		ssize_t sent = sendto(server_socket, (const char*)resp_buf, (int)sizeof(resp_buf), 0, (struct sockaddr*)&client_addr, client_len);
		if (sent != (ssize_t)sizeof(resp_buf)) {
			fprintf(stderr, "Errore invio risposta al client %s (ip %s).\n", client_name, client_ip);
		}
	}

	closesocket(server_socket);
	clearwinsock();
	return 0;

}
