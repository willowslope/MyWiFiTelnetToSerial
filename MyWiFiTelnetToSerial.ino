#include <ESP8266WiFi.h>

//Sleep Timer
#define SLEEP_TIMER (unsigned long)(30*60*1000)
unsigned long time_old = 0;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1
const char *ssid = "ESP_OBDII";
const char *password = "";
WiFiServer server(35000);
WiFiClient serverClients[MAX_SRV_CLIENTS];
void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 0, 10), IPAddress(192, 168, 0, 10), IPAddress(255, 255, 255, 0));
//  WiFi.softAP(ssid, password);
  WiFi.softAP(ssid);
  //start UART and the server
  Serial.begin(38400);
  server.begin();
  server.setNoDelay(true);

    
}
void loop() {
  uint8_t i;
  //check Sleep Timer
  if ((unsigned long)(millis()-time_old)>SLEEP_TIMER)
  {
    ESP.deepSleep(0); 
  }
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
//        serverClients[i].println("Telnet Start(WiFi)");
//        Serial.println("Telnet Start(COM)");
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        time_old = millis();
        //get data from the telnet client and push it to the UART
        while(serverClients[i].available()) Serial.write(serverClients[i].read());
      }
    }
  }
  //check UART for data
  if(Serial.available()){
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
      serverClients[i].write(sbuf, len);
      delay(1);
      }
    }
  }
}

