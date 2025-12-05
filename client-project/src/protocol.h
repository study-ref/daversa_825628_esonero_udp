/*
 * protocol.h
 *
 * Client header file
 * Definitions, constants and function prototypes for the client
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

// Porta di default
#define SERVER_PORT 56700

// Lunghezza massima del nome della città (incluso terminatore '\0')
#define CITY_NAME_LEN 25

// Codici di stato
#define STATUS_OK 0
#define STATUS_CITY_NOT_AVAILABLE 1
#define STATUS_INVALID_REQUEST 2

// Strutture
typedef struct {
	char type;
	char city[CITY_NAME_LEN];
} weather_request_t;

typedef struct {
	int status;
	char type;
	float value;
} weather_response_t;

// Funzioni ausiliarie (ricezione, invio e generazione valori nel server)
static int send_all(int sock, const void *buf, int len) {
	const char *p = (const char*)buf;
	int sent = 0;
	while (sent < len) {
		int r = send(sock, p + sent, len - sent, 0);
		if (r <= 0)
			return -1;
		sent += r;
	}
	return sent;
}

static int recv_all(int sock, void *buf, int len) {
	char *p = (char*)buf;
	int recvd = 0;
	while (recvd < len) {
		int r = recv(sock, p + recvd, len - recvd, 0);
		if (r <= 0)
			return -1;
		recvd += r;
	}
	return recvd;
}

float get_temperature();
float get_humidity();
float get_wind();
float get_pressure();

#endif /* PROTOCOL_H_ */
