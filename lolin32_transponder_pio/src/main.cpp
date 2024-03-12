#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>

#include <Wire.h>
#include <Adafruit_BME280.h>

// letzte version meines transponders mit bme280 und esp und einem http-server auf meinem beelink

typedef struct
{
  int transponderID;
  float temp;
  float hum;
  float pressure;
} dataframe;

void sendDataToServer(String measDataString);
void initBME280();
dataframe getBME280Readings();
void deepSleep();
String dataframeToString(dataframe d);

void sendHttpGetRequest();

/* !!!! id is set here. It is unique and has to be set here before flashing new sensorunit !!!! */
const int id = 1;

// wifi data
const char *ssid = "TP-Link_4D9A";
const char *password = "78811928";

// server data
const char *server = "192.168.1.119"; // ip of butzdigga-server
const int port = 8080;
const char *filename = "/index.html";
const char *url = "192.168.1.119:8080/index.html";

// BME stuff
Adafruit_BME280 bme;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  initBME280();
}

void loop()
{
  dataframe measData = {id, 0, 0, 0};
  measData = getBME280Readings();
  delay(10);

  /* Connect to Wifi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nlocal IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("gateway IP: ");
  Serial.println(WiFi.gatewayIP());

  Serial.println(dataframeToString(measData));

  sendDataToServer(dataframeToString(measData));

  delay(10);
  WiFi.disconnect();
  deepSleep();
}

void initBME280()
{
  Wire.begin(23, 19);
  Adafruit_BME280 b;
  int status = b.begin(0x76, &Wire);
  bme = *(&b);
  delay(10);
}

dataframe getBME280Readings()
{
  return {id, bme.readTemperature(), bme.readHumidity(), bme.readPressure()};
}

/* Builds a POST Request and sends it to the server */
void sendDataToServer(String measDataString)
{

  // Verbindung zum Server herstellen
  HTTPClient http;
  http.begin("http://" + String(server) + ":" + String(port) + "/measData.txt");
  http.addHeader("Content-Type", "text/plain");

  // HTTP POST-Anfrage senden
  int httpResponseCode = http.POST(measDataString);

  // Antwort des Servers ausgeben
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  }
  else
  {
    Serial.print("HTTP POST failed, error: ");
    Serial.println(httpResponseCode);
  }

  // Verbindung schließen
  http.end();
}

String dataframeToString(dataframe d)
{
  return (String(d.transponderID, 2) + "," + String(d.temp, 2) + "," + String(d.hum, 2) + "," + String(d.pressure, 2));
}

void sendHttpGetRequest()
{
  // Verbindung zum Server herstellen
  WiFiClient client;

  if (!client.connect(server, port))
  {
    Serial.println("Connection failed");
    return;
  }

  // HTTP-Anfrage senden
  client.print("GET /index.html HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(server);
  client.print("\r\n");
  client.print("Connection: close\r\n");
  client.print("\r\n");

  // Antwort vom Server empfangen und ausgeben
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  // Verbindung schließen
  client.stop();
}

void deepSleep()
{
  esp_sleep_enable_timer_wakeup(10000000); // * 1e-6 eg microseconds
  esp_deep_sleep_start();
}