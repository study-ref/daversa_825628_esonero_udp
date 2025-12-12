/*
 * protocol.h
 *
 * Server header file
 * Definitions, constants and function prototypes for the server
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define SERVER_PORT 56700

// Lunghezza massima del nome della città (incluso terminatore '\0')
#define CITY_NAME_LEN 64

// Lunghezza della richiesta e della risposta
#define REQUEST_LEN   (1 + CITY_NAME_LEN)	// type (1) + city (CITY_NAME_LEN)
#define RESPONSE_LEN  (4 + 1 + 4)			// status (4) + type (1) + value (4)

// Offset simbolici per il buffer di risposta
#define RESP_STATUS_OFFSET 0
#define RESP_TYPE_OFFSET   4
#define RESP_VALUE_OFFSET  5

// Codici di stato
#define STATUS_OK 0
#define STATUS_CITY_NOT_AVAILABLE 1
#define STATUS_INVALID_REQUEST 2

// Dimensione della coda di listen
#define QUEUE_SIZE 5

// Strutture
typedef struct {
	char type;
	char city[CITY_NAME_LEN];
} weather_request_t;

typedef struct {
	unsigned int status;
	char type;
	float value;
} weather_response_t;

/* Prototipo funzione esempio
int connect_to_server(const char* server_address); */

static int city_supported(const char *city) {
	if (!city)
		return 0;
	char tmp[CITY_NAME_LEN];
	strncpy(tmp, city, CITY_NAME_LEN-1);
	tmp[CITY_NAME_LEN-1] = '\0';
	for (char *p = tmp; *p; ++p)
		*p = (char)toupper((unsigned char)*p);
	const char *cities[] = {"BARI","ROMA","MILANO","NAPOLI","TORINO","PALERMO","GENOVA","BOLOGNA","FIRENZE","VENEZIA"};
	for (size_t i = 0; i < sizeof(cities)/sizeof(cities[0]); ++i) {
		if (strcmp(tmp, cities[i]) == 0)
			return 1;
	}
	return 0;
}

float get_temperature() {
	float r = (float)rand() / RAND_MAX;
	return -10.0f + r*(50.0f);
}

float get_humidity() {
	float r = (float)rand() / RAND_MAX;
	return 20.0f + r*(80.0f);
}

float get_wind() {
	float r = (float)rand() / RAND_MAX;
	return r*100.0f;
}

float get_pressure() {
	float r = (float)rand() / RAND_MAX;
	return 950.0f + r*100.0f;
}

#endif /* PROTOCOL_H_ */
