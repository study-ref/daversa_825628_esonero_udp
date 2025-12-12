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
#include <ctype.h>
#include <stdint.h>
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#include "protocol.h"

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}


int main(int argc, char *argv[]) {
	// Parsing argomenti
	const char *server_address = "localhost";
	int port = SERVER_PORT;
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
		fprintf(stderr, "Errore in WSAStartup()\n");
		return -1;
	}
#endif

	// Server (supportati sia hostname che indirizzo IP)
	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port);
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int get_addr_info = getaddrinfo(server_address, port_str, &hints, &res);
	if (get_addr_info != 0 || res == NULL) {
#if defined WIN32
		if (get_addr_info != 0) {
			fprintf(stderr, "Risoluzione server fallita: %d\n", get_addr_info);
		} else {
			fprintf(stderr, "Risoluzione server fallita: nessun risultato.\n");
		}
#else
		if (get_addr_info != 0) {
			fprintf(stderr, "Risoluzione server fallita: %s\n", gai_strerror(get_addr_info));
		} else {
			fprintf(stderr, "Risoluzione server fallita: nessun risultato.\n");
		}
#endif
		clearwinsock();
		return -1;
	}

	// Estrazione IP e reverse lookup per il nome
	char server_ip[INET_ADDRSTRLEN] = {0};
	struct sockaddr_in *sin = (struct sockaddr_in*)res->ai_addr;
	inet_ntop(AF_INET, &sin->sin_addr, server_ip, sizeof(server_ip));

	char server_name[NI_MAXHOST] = {0};
	if (getnameinfo((struct sockaddr*)res->ai_addr, res->ai_addrlen, server_name, sizeof(server_name), NULL, 0, 0) != 0) {
		// Fallback: uso della stringa fornita dall'utente
		strncpy(server_name, server_address, sizeof(server_name)-1);
		server_name[sizeof(server_name)-1] = '\0';
	}

	// Creazione socket UDP
	int my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (my_socket < 0) {
		fprintf(stderr, "Creazione della socket UDP fallita.\n");
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Parsing e validazione della richiesta (-r)
	if (!req_str) {
		fprintf(stderr, "In uso: %s [-s server] [-p port] -r \"type city\"\n", argv[0]);
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Il -r può contenere spazi ma non ammette tab
	for (const char *pp = req_str; *pp; ++pp) {
		if (*pp == '\t') {
			fprintf(stderr, "Errore: la richiesta non può contenere tabulazioni.\n");
			closesocket(my_socket);
			freeaddrinfo(res);
			clearwinsock();
			return -1;
		}
	}

	// Primo token (fino al primo spazio) deve essere un singolo carattere
	const char *space = strchr(req_str, ' ');
	if (!space) {
		fprintf(stderr, "Richiesta non valida: formato atteso 'type city'.\n");
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}
	size_t token_len = (size_t)(space - req_str);
	if (token_len != 1) {
		fprintf(stderr, "Richiesta non valida: il primo token deve essere un singolo carattere.\n");
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	char type = req_str[0];
	// city = resto della stringa dopo il primo spazio (si saltano spazi aggiuntivi iniziali)
	const char *city_src = space + 1;
	while (*city_src && isspace((unsigned char)*city_src))
		++city_src;
	if (!*city_src) {
		fprintf(stderr, "Richiesta non valida: manca il nome della città.\n");
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Validazione lunghezza city
	char city[CITY_NAME_LEN];
	strncpy(city, city_src, CITY_NAME_LEN-1);
	city[CITY_NAME_LEN-1] = '\0';
	if (strlen(city_src) >= CITY_NAME_LEN) {
		fprintf(stderr, "Errore: nome città troppo lungo (max %d).\n", CITY_NAME_LEN-1);
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Preparazione buffer richiesta (serializzazione)
	unsigned char req_buf[1 + CITY_NAME_LEN];
	memset(req_buf, 0, sizeof(req_buf));
	req_buf[0] = type;
	memcpy(req_buf + 1, city, strlen(city) + 1);

	// Invio tramite sendto
	ssize_t sent = sendto(my_socket, (const char*)req_buf, sizeof(req_buf), 0, res->ai_addr, (int)res->ai_addrlen);
	if (sent != (ssize_t)sizeof(req_buf)) {
		fprintf(stderr, "Errore invio richiesta.\n");
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Ricezione risposta
	unsigned char resp_buf[RESPONSE_LEN];
	struct sockaddr_storage from_addr;
	socklen_t from_len = sizeof(from_addr);
	ssize_t r = recvfrom(my_socket, (char*)resp_buf, sizeof(resp_buf), 0, (struct sockaddr*)&from_addr, &from_len);
	if (r != (ssize_t)sizeof(resp_buf)) {
		fprintf(stderr, "Errore ricezione risposta.\n");
		closesocket(my_socket);
		freeaddrinfo(res);
		clearwinsock();
		return -1;
	}

	// Deserializzazione risposta
	uint32_t net_status;
	memcpy(&net_status, resp_buf, sizeof(net_status));
	uint32_t status = ntohl(net_status);
	char resp_type = resp_buf[RESP_TYPE_OFFSET];
	uint32_t net_val_bits;
	memcpy(&net_val_bits, resp_buf + RESP_VALUE_OFFSET, sizeof(net_val_bits));
	net_val_bits = ntohl(net_val_bits);
	float response_value;
	memcpy(&response_value, &net_val_bits, sizeof(response_value));

	// Stampa risultato con nome server (reverse lookup) e IP
	printf("Ricevuto risultato dal server %s (ip %s). ", server_name, server_ip);

	if (status == STATUS_OK) {
		// Standardizzazione output
		if (city[0]) {
			city[0] = (char)toupper((unsigned char)city[0]);
			for (size_t i = 1; city[i]; ++i)
        		city[i] = (char)tolower((unsigned char)city[i]);
		}
		if (resp_type == 't') {
			printf("%s: Temperatura = %.1f°C\n", city, response_value);
		} else if (resp_type == 'h') {
			printf("%s: Umidità = %.1f%%\n", city, response_value);
		} else if (resp_type == 'w') {
			printf("%s: Vento = %.1f km/h\n", city, response_value);
		} else if (resp_type == 'p') {
			printf("%s: Pressione = %.1f hPa\n", city, response_value);
		}
	} else if (status == STATUS_CITY_NOT_AVAILABLE) {
		printf("%s", "Città non disponibile\n");
	} else if (status == STATUS_INVALID_REQUEST) {
		printf("%s", "Richiesta non valida\n");
	} else {
		printf("%s", "Errore\n");
	}

	closesocket(my_socket);
	freeaddrinfo(res);
	clearwinsock();
	return 0;

}



