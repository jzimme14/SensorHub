// networking includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
// c includes
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
// own file includes
#include "sqlite/sqlite3.h"

#define _GNU_SOURCE

// dataframe to hold a data-stamp from a specific transponder (date_and_time is added by the server at connection time)
typedef struct
{
	int transponderID;
	float temp;
	float hum;
	float pressure;
} dataframe;

typedef struct
{
	char type[10];
	int content_length;
	char connection[20];
	char content_type[20];
	char target_file[20];
	char *payload;
	char acceptedFormat[10];
	char parameter[15];
	int parameter_int;
} HTTP_Request;

// prototypes
int saveDataToDatabase(dataframe d);
int getDataFromDatabase(int transponderID, char *resultString);
HTTP_Request http_request_constr(char *buffer);
void print_visible_characters(const char *str);
dataframe ReceivedData_into_Dataframe(char *s);

int main(void)
{
	// run variable
	bool running = true;

	// create special instance of sockaddr_in
	struct sockaddr_in addr = {AF_INET, 0x901F, 0};

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	printf("\nbinding socket");
	fflush(stdout);
	while (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		printf(".");
		fflush(stdout);
		sleep(1);
	}

	printf("\nbind successful! \n");

	while (running)
	{
		listen(socket_fd, 10);

		int client_fd = accept(socket_fd, 0, 0);
		if (client_fd == -1)
		{
			fprintf(stderr, "accept failed...");
			exit(-10);
		}

		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		if (recv(client_fd, buffer, 1024, 0) == -1)
		{
			fprintf(stderr, "recv failed...");
			exit(-4);
		}
		printf("\n\n buffer: \n%s\n\n", buffer);
		fflush(stdout);

		// http header interface
		HTTP_Request http_request = http_request_constr(buffer);
		// print_visible_characters(buffer);
		printf("type: %s\ncontent-length: %d\nconnection: %s\ncon-type: %s \ntarget-dir: %s \naccepted-format: %s\nparameter: %s\n\n", http_request.type, http_request.content_length, http_request.connection, http_request.content_type, http_request.target_file, http_request.acceptedFormat, http_request.parameter);

		if (strcmp(http_request.type, "GET") == 0)
		{
			printf("Entered GET section\n");

			if (strcmp(http_request.target_file, "close_socket") == 0)
			{
				printf("\nsocket is closed! \n");
				running = false;
			}
			else if (strcmp(http_request.target_file, "measData.db") == 0)
			{
				char dete[30];
				int deteRetVal = 0;
				memset(dete, 0, sizeof(dete));
				deteRetVal = getDataFromDatabase(http_request.parameter_int, dete);
				if (deteRetVal == 0)
				{
					char response[200] = "HTTP/1.1 200 OK\r\n\r\n";
					if (strcat(response, dete) == response)
					{
						send(client_fd, response, strlen(response), 0);
						printf("\nData was sent: \n%s\n", response);
					}
					else
					{
						fprintf(stderr, "strcat error...");
						exit(-1010);
					}
				}
				else
				{
					send(client_fd, "HTTP/1.1 404 NO\r\n\r\nHTTP/1.1 404 NO", strlen("HTTP/1.1 404 NO\r\n\r\nHTTP/1.1 404 NO"), 0);
				}
			}
			else
			{
				// file handle of opened html-data
				int opened_fd = open(http_request.target_file, O_RDONLY);
				if (opened_fd == -1)
				{
					char response[] = "HTTP/1.1 404 NotFound\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n404 Not Found";
					send(client_fd, response, strlen(response), 0);
				}
				else
				{
					printf("file to read: %s\n", http_request.target_file);

					// send file to client

					char responseHeaderLine[] = "HTTP/1.1 200 ok\r\ncontent-type: %s\r\n\r\n";
					int responseLength = strlen(responseHeaderLine) + 10;
					char *response = (char *)malloc(responseLength + 1); // +1 für das Nullzeichen am Ende
					memset(response, 0, sizeof(responseLength + 1));
					sprintf(response, responseHeaderLine, http_request.acceptedFormat);
					printf("\nResponse: %s\n", response, responseLength);

					// send desired data and response back to client
					send(client_fd, response, strlen(response), 0);
					sendfile(client_fd, opened_fd, 0, 65400);

					// close file descriptor of data and free dynamical memory
					close(opened_fd);
					free(response);
				}
			}
		}
		else if (strcmp(http_request.type, "POST") == 0)
		{
			http_request.payload = (char *)malloc(http_request.content_length + 1);
			memset(http_request.payload, 0, http_request.content_length + 1);
			if (http_request.payload == NULL)
			{
				fprintf(stderr, "malloc failed for http_req.payload...");
				exit(-6);
			}

			if (read(client_fd, http_request.payload, http_request.content_length) == -1)
			{
				fprintf(stderr, "reading POST-payload failed...");
				exit(-7);
			}

			printf("read output: %s\n", http_request.payload);

			// Put received data into fitting dataframe/format
			dataframe payload;
			memset(&payload, 0, sizeof(dataframe));
			payload = ReceivedData_into_Dataframe(http_request.payload);

			// save data to sqlite-database
			int db_operation_retval = saveDataToDatabase(payload);

			if (db_operation_retval == 0)
			{
				char *response = "HTTP/1.1 200 OK\r\n\r\nHTTP/1.1 200 OK";
				if (send(client_fd, response, strlen(response), 0) == -1)
				{
					fprintf(stderr, "sending ok response failed...");
					exit(-8);
				}
			}
			else
			{
				char *response = "HTTP/1.1 404 NO\r\n\r\nHTTP/1.1 404 NO";
				if (send(client_fd, response, strlen(response), 0) == -1)
				{
					fprintf(stderr, "sending no response failed...");
					exit(-11);
				}
			}

			free(http_request.payload);
		}
		else
		{
			fprintf(stderr, "\naccessed using unknown http-request...");
		}
		// close client connection
		if (close(client_fd) != 0)
		{
			fprintf(stderr, "closing client connection failed...");
			exit(-5);
		}
	}

	// close socket
	if (close(socket_fd) != 0)
	{
		fprintf(stderr, "closing socket failed...");
	}

	return 0;
}

dataframe ReceivedData_into_Dataframe(char *s)
{
	dataframe d;
	memset(&d, 0, sizeof(dataframe));

	int retVal = sscanf(s, "%d,%f,%f,%f", &d.transponderID, &d.temp, &d.hum, &d.pressure);
	if (retVal == 4)
	{
		return d;
	}
	else
	{
		fprintf(stderr, "read error occured! ");
		exit(11);
	}
}

int saveDataToDatabase(dataframe d)
{

	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql;

	/* Open database */
	rc = sqlite3_open("measData.db", &db);

	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return (-1);
	}

	/* Create Table for Measurements if it wasn't already created */
	const char *create_table_sql = "CREATE TABLE IF NOT EXISTS Measurements (MeasNumber INTEGER PRIMARY KEY, ID INTEGER, temp REAL, hum REAL, pressure REAL, date_time TEXT);";
	rc = sqlite3_exec(db, create_table_sql, 0, 0, &zErrMsg);

	/* Put Data into Database */
	// Get Time from sqlite natively by using 'SELECT datetime('now','localtime')'
	char insert_data_sql[200];
	const char *insert_data_sql_blueprint = "INSERT INTO Measurements (ID, temp, hum, pressure, date_time) VALUES (%d, %.2f, %.2f, %.2f, datetime('now','localtime'));";
	sprintf(insert_data_sql, insert_data_sql_blueprint, d.transponderID, d.temp, d.hum, d.pressure);
	rc = sqlite3_exec(db, insert_data_sql, 0, 0, &zErrMsg);

	if (rc)
	{
		fprintf(stderr, "cant add data: %s\n", sqlite3_errmsg(db));
		return (-2);
	}

	sqlite3_close(db);

	return 0;
}

int getDataFromDatabase(int transponderID, char *resultString)
{
	sqlite3 *db;
	sqlite3_stmt *stmt;

	char sql_query[300];
	memset(sql_query, 0, sizeof(sql_query));

	char result[50];
	memset(result, 0, sizeof(result));

	// Öffne die Datenbankverbindung
	int rc = sqlite3_open("measData.db", &db);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}

	sprintf(sql_query, "SELECT * FROM Measurements WHERE ID = %d ORDER BY date_time DESC LIMIT 1;", transponderID);

	rc = sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		return 1;
	}

	double temp = 0, hum = 0, pressure = 0;
	const unsigned char *date_time;

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		temp = sqlite3_column_double(stmt, 2);
		hum = sqlite3_column_double(stmt, 3);
		pressure = sqlite3_column_double(stmt, 4) / 100; // convert from Pa -> hPa
		date_time = sqlite3_column_text(stmt, 5);
	}
	else
	{
		fprintf(stderr, "error while reading database...");
		exit(-10);
	}

	sprintf(result, "%d,%.2f,%.2f,%.2f,%s", transponderID, temp, hum, pressure, date_time);

	strcpy(resultString, result);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return 0;
}

// Blueprint for a http-header. Is filled after receiving raw header string. Used to access specific Header-Specifiers
HTTP_Request http_request_constr(char *buffer)
{
	HTTP_Request h;
	memset(&h, 0, sizeof(HTTP_Request));

	// get http request type
	const char post_str[] = "POST";
	const char get_str[] = "GET";

	if (strstr(buffer, "POST") != NULL)
	{
		strcpy(h.type, post_str);
	}
	else if (strstr(buffer, "GET") != NULL)
	{
		strcpy(h.type, get_str);
	}

	// get length of buffer
	int buflen = strlen(buffer);
	buflen += 1;

	// get content-length
	char *ubuf = (char *)malloc(buflen);
	if (ubuf != NULL)
	{
		strcpy(ubuf, buffer);
		char *clptr = strstr(ubuf, "Content-Length:");
		if (clptr != NULL)
		{
			clptr = clptr + 16;
			char *endptr = strchr(clptr, '\r');
			if (endptr != NULL)
			{
				// Nullterminiere den String, damit atoi nur die Zahl liest
				*endptr = '\0';
				// Konvertiere die Zahl in einen Integer
				h.content_length = atoi(clptr);
			}
		}
		else
		{
			h.content_length = 0;
		}
		free(ubuf);
	}
	else
	{
		fprintf(stderr, "malloc failed...");
	}

	// connection type
	char *ubuf1 = (char *)malloc(buflen);
	if (ubuf1 != NULL)
	{
		strcpy(ubuf1, buffer);
		char *conptr = strstr(ubuf1, "Connection:");
		if (conptr != NULL)
		{
			conptr = conptr + 12;
			*strchr(conptr, '\r') = 0;
			strcpy(h.connection, conptr);
		}
		else
		{
			strcpy(h.connection, "n");
		}
		free(ubuf1);
	}
	else
	{
		fprintf(stderr, "malloc failed...");
	}

	// get content type
	char *ubuf2 = (char *)malloc(buflen);
	if (ubuf2 != NULL)
	{
		strcpy(ubuf2, buffer);
		char *contypeptr = strstr(ubuf2, "Content-Type: ");
		if (contypeptr != NULL)
		{
			contypeptr += 14;
			*strchr(contypeptr, '\r') = 0;
			strcpy(h.content_type, contypeptr);
		}
		else
		{
			strcpy(h.connection, "n");
		}
		free(ubuf2);
	}
	else
	{
		fprintf(stderr, "malloc failed...");
	}

	// get target directory
	char *helper1 = NULL;
	char *helper2 = NULL;
	char *ihi = NULL;
	char *ubuf3 = (char *)malloc(buflen);
	if (ubuf3 != NULL)
	{
		strcpy(ubuf3, buffer);
		char *tardatptr = ubuf3;
		if (strcmp(h.type, "POST") == 0)
		{
			tardatptr += 6;
		}
		else if (strcmp(h.type, "GET") == 0)
		{
			tardatptr += 5;
		}

		helper1 = strchr(tardatptr, ' ');
		helper2 = strchr(tardatptr, '?');

		if (helper1 != NULL && helper2 != NULL)
		{
			if (helper1 <= helper2)
			{
				*helper1 = 0;
				strcpy(h.target_file, tardatptr);
			}
			else
			{
				*helper2 = 0;
				strcpy(h.target_file, tardatptr);
				// get parameter
				helper2 += 1;
				ihi = strchr(helper2, ' ');
				if (ihi != NULL)
				{
					*ihi = 0;
					strcpy(h.parameter, helper2);
					h.parameter_int = atoi(h.parameter);
				}
			}
		}
		else if (helper1 == NULL && helper2 != NULL)
		{
			*helper2 = 0;
			strcpy(h.target_file, tardatptr);
			// get parameter
			helper2 += 1;
			ihi = strchr(helper2, ' ');
			if (ihi != NULL)
			{
				*ihi = 0;
				strcpy(h.parameter, helper2);
				h.parameter_int = atoi(h.parameter);
			}
		}
		else if (helper2 == NULL && helper1 != NULL)
		{
			*helper1 = 0;
			strcpy(h.target_file, tardatptr);
		}
		else
		{
		}

		free(ubuf3);
	}
	else
	{
		fprintf(stderr, "malloc failed...");
	}

	// get accepted format
	char *ubuf4 = (char *)malloc(buflen);
	if (ubuf4 != NULL)
	{
		strcpy(ubuf4, buffer);
		char *aformatptr = strstr(ubuf4, "Accept: ");
		if (aformatptr != NULL)
		{
			aformatptr = strstr(aformatptr, "text");
			if (aformatptr != NULL)
			{
				char *i = strchr(aformatptr, ',');
				char *j = strchr(aformatptr, '\r');
				if ((j - aformatptr) < (i - aformatptr))
				{
					*j = 0;
				}
				else if ((j - aformatptr) > (i - aformatptr))
				{
					*i = 0;
				}

				strcpy(h.acceptedFormat, aformatptr);
			}
		}
		free(ubuf4);
	}
	else
	{
		fprintf(stderr, "malloc failed...");
	}

	return h;
}

void print_visible_characters(const char *str)
{
	// Gehe durch jeden Zeichen im String
	for (int i = 0; str[i] != '\0'; i++)
	{
		// Überprüfe, ob das Zeichen ein unsichtbares Zeichen ist
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
		{
			// Gib das unsichtbare Zeichen als Escape-Sequenz aus
			switch (str[i])
			{
			case ' ':
				printf("' ' (Space)\n");
				break;
			case '\t':
				printf("\\t (Tab)\n");
				break;
			case '\n':
				printf("\\n (Newline)\n");
				break;
			case '\r':
				printf("\\r (Carriage Return)\n");
				break;
			// Füge weitere unsichtbare Zeichen nach Bedarf hinzu
			default:
				printf("Unknown invisible character\n");
				break;
			}
		}
		else
		{
			// Gib sichtbare Zeichen direkt aus
			printf("%c\n", str[i]);
		}
	}
}