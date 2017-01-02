/*
  RC Hub
  
  Written in 2016 by Jens Olsson.
*/

// Import required libraries
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include "HomeEasy.h"

#include "wifiparams.h"

// Create Serial for the WiFi card
SoftwareSerial Serial1(6, 7);

int status = WL_IDLE_STATUS;

// Create an HomeEasy instance
HomeEasy homeEasy;

int nbrCalls = 3;

// Default NEXA remote ID
unsigned long defaultRemoteId = 11111111; //22611714;

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiEspServer server(LISTEN_PORT);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  Serial1.begin(9600);

  // Initate HomeEasy
  homeEasy = HomeEasy();
  
  homeEasy.registerSimpleProtocolHandler(printSimpleResult);
  homeEasy.registerAdvancedProtocolHandler(printAdvancedResult);
  
  homeEasy.init();

  // Initiate WiFi
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");

    while(true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attemting to connect to SSID: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWiFiStatus();

  // Start the server
  server.begin();
}

void loop() {

  // Handle REST calls
  WiFiEspClient client = server.available();

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        process(client);
      }
  
      delay(1);
  
      client.stop();

      while(client.status() != 0) {
        delay(5);
      }
    }
  }
}

void process(WiFiEspClient client) {

  // Jump over HTTP action
  client.readStringUntil('/');

  // Get the command
  String command = client.readStringUntil('/');

  if (command == "SwitchOn") {
    switchOnOff(true, client);
  }
  else if (command == "SwitchOff") {
    switchOnOff(false, client);
  }
  else {
    Serial.print("Received unknown command: ");
    Serial.println(command);

    client.print(
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/html\r\n"
            "\r\n");
    return;
  }

  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n");
}

// Custom function accessible by the API
int switchOnOff(bool on, WiFiEspClient client) {

  // Get first param
  unsigned int device = client.parseInt();

  unsigned long remoteId = (unsigned long)client.parseInt();
  
  if (remoteId < 10000000) {
    remoteId = defaultRemoteId;
  }

  client.flush();
  
  bool group = false;

  Serial.print("switchOnOff() called: ");
  Serial.print(remoteId);
  Serial.print(":");
  Serial.print(device);
  Serial.print(":");
  Serial.println(on);

  for(int i=0; i < nbrCalls; i++) {
    homeEasy.sendAdvancedProtocolMessage(remoteId, device, on, group);
  }
}

/**
 * Print the details of the advanced protocol message.
 */
void printAdvancedResult(unsigned long sender, unsigned int recipient, bool on, bool group)
{
  Serial.println("advanced protocol message");
  
  Serial.print("sender ");
  Serial.println(sender);
  
  Serial.print("recipient ");
  Serial.println(recipient);
  
  Serial.print("on ");
  Serial.println(on);
  
  Serial.print("group ");
  Serial.println(group);
  
  Serial.println();
}


/**
 * Print the details of the simple protocol message.
 */
void printSimpleResult(unsigned int sender, unsigned int recipient, bool on)
{
  Serial.println("simple protocol message");
  
  Serial.print("sender ");
  Serial.println(sender);
  
  Serial.print("recipient ");
  Serial.println(recipient);
  
  Serial.print("on ");
  Serial.println(on);
  
  Serial.println();
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("AP Address: ");
  Serial.println(ip);
}
