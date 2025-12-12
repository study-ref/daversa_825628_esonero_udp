/*
 * protocol.h
 *
 * Client header file
 * Definitions, constants and function prototypes for the client
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
#define RESP_TYPE_OFFSET   4
#define RESP_VALUE_OFFSET  5

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
	unsigned int status;
	char type;
	float value;
} weather_response_t;

// Prototipi (server implementa queste funzioni)
float get_temperature();
float get_humidity();
float get_wind();
float get_pressure();

#endif /* PROTOCOL_H_ */