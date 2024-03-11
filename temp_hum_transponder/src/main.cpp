#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>

// letzte version meines transponders mit bme280 und esp und einem http-server auf meinem beelink

void deepSleep();
void sendHttpRequest();
void sendHttpPostRequest();

typedef struct
{
  int transponderID;
  float temp;
  float hum;
  float pressure;
} dataframe;

// wifi data
const char *ssid = "TP-Link_4D9A";
const char *password = "78811928";

// server data
const char *server = "192.168.1.119"; // ip of butzdigga-server
const int port = 8080;
const char *filename = "/index.html";

const char *url = "192.168.1.119:8080/index.html";

void setup()
{
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("gateway IP: ");
  Serial.println(WiFi.gatewayIP());

  sendHttpRequest();
  delay(2000);
  sendHttpPostRequest();
}

void loop()
{
}

void sendHttpPostRequest()
{
  // Daten, die an den Server gesendet werden sollen
  String postData = "hello";

  // Verbindung zum Server herstellen
  HTTPClient http;
  http.begin("http://" + String(server) + ":" + String(port) + "/measData.txt");
  http.addHeader("Content-Type", "text/plain");

  // HTTP POST-Anfrage senden
  int httpResponseCode = http.POST(postData);

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

void sendHttpRequest()
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
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, HIGH);
  esp_deep_sleep_start();
}