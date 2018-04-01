// ****************************************************************
// MyWiFiTelnetToSerial
//  Ver0.02 2018.3.3
//  copyright Sakayanagi
// ****************************************************************

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
// ****************************************************************
// * Const, RAM, define
// ****************************************************************
const char *HostName = "ESP_OBDII";
//Sleep Timer 30min
#define SLEEP_TIMER (unsigned long)(30*60*1000)
unsigned long timer_sleep = 0;
//unsigned long timer_1s = 0;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1
WiFiServer server(35000);
WiFiClient serverClients[MAX_SRV_CLIENTS];

ESP8266WebServer websvr(80);  //web server

String ap_ssid = "ESP_OBDII";
String ap_pass = "";
char sta_ssid[33];
char sta_pass[64];

#define EEPROM_NUM (sizeof(sta_ssid)+sizeof(sta_pass))

#if 0
#define DEBUG_PRINT(x) Serial.println(x);
#else
#define DEBUG_PRINT(x)
#endif

// ****************************************************************
// * Prototype
// ****************************************************************

// ****************************************************************
// * support
// ****************************************************************
void set_EEPROM(void){
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i=0;i<sizeof(sta_ssid); i++)  {*pd = sta_ssid[i]; pd++;}
  for (i=0;i<sizeof(sta_pass); i++)  {*pd = sta_pass[i]; pd++;}
  for (i = 0; i < EEPROM_NUM; i++) {EEPROM.write(i, data[i]);}
  EEPROM.commit();
}

// ****************************************************************
// * HomePage
// ****************************************************************
void handleTopPage() {
  String html = "\
<!DOCTYPE html><html><head>\
<title>Setting</title>\
<style> h1{background:black;color:white;}</style>\
</head><body>\
<H1>ESP_OBDII Setting</H1>\
<form name='inputform' action='' method='POST'>\
<H2>STA Setting</H2>\
SSID:<input type='text' name='sta_ssid' maxlength='32' value='**sta_ssid'><br>\
PASS:<input type='text' name='sta_pass' maxlength='63' value='**sta_pass'><br>\
<br><input type='submit' name='Set_ID' value='Set'>\
</form><hr>\
<H2>Status</H2>\
<table border='1'>\
<tr><th>mode</th><th>SSID      </th><th>PASS      </th><th>IP</th></tr>\
<tr><td>AP  </td><td>**ap_ssid </td><td>**ap_pass </td><td>**ap_IP </td></tr>\
<tr><td>STA </td><td>**sta_ssid</td><td>**sta_pass</td><td>**sta_IP</td></tr>\
</table>\
</body></html>";

  if (websvr.hasArg("Set_ID")) {
    if (websvr.hasArg("sta_ssid")) strcpy(sta_ssid,websvr.arg("sta_ssid").c_str());
    if (websvr.hasArg("sta_pass")) strcpy(sta_pass,websvr.arg("sta_pass").c_str());
    set_EEPROM();
  }

  html.replace("**ap_ssid",ap_ssid);
  html.replace("**ap_pass",ap_pass);
  html.replace("**ap_IP",WiFi.softAPIP().toString());
  html.replace("**sta_ssid",sta_ssid);
  html.replace("**sta_pass",sta_pass);
  if(WiFi.status() == WL_CONNECTED) html.replace("**sta_IP",WiFi.localIP().toString());
  else html.replace("**sta_IP","Disconnect");

  websvr.send(200, "text/html", html);
}

// -----------------------------------
// Not Found
//------------------------------------
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += websvr.uri();
  message += "\nMethod: ";
  message += (websvr.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += websvr.args();
  message += "\n";
  for (uint8_t i = 0; i < websvr.args(); i++) {
    message += " " + websvr.argName(i) + ": " + websvr.arg(i) + "\n";
  }
  websvr.send(404, "text/plain", message);
}
// ****************************************************************
// * setup
// ****************************************************************

void setup_ram(void){
        timer_sleep = millis();
        ap_ssid = ap_ssid + "_" + String(ESP.getChipId(),HEX);
}

// ------------------------------------
void setup_eeprom(void) {
  //EEPROM
  EEPROM.begin(EEPROM_NUM);
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i = 0; i < EEPROM_NUM; i++) {data[i] = EEPROM.read(i);}
  for (i=0;i<sizeof(sta_ssid);i++){sta_ssid[i] = *pd; pd++;}
  for (i=0;i<sizeof(sta_pass);i++){sta_pass[i] = *pd; pd++;}
}

// ------------------------------------
void setup_com(void){
  Serial.begin(38400);
}
// ------------------------------------
void setup_mDNS(void) {
  if(WiFi.status() == WL_CONNECTED){
    WiFi.hostname(HostName);
    if (!MDNS.begin(HostName,WiFi.localIP())) {
      MDNS.addService("telnet", "tcp", 35000);
      MDNS.addService("http", "tcp", 80);
    }
  }
}
// ------------------------------------
void setup_wifi(void){
  if (strlen(sta_ssid)>0){
    DEBUG_PRINT("APSTAmode")
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(sta_ssid,sta_pass);
 
    unsigned long t_time = millis(); 
    while(WiFi.status() != WL_CONNECTED)
    {
      if (millis() > t_time + 15000){      //15秒だけ待つ
        WiFi.disconnect();
        break;
      }
      delay(500);
    }
  }else{
    DEBUG_PRINT("APmode")
    WiFi.mode(WIFI_AP);
  }

  WiFi.softAPConfig(IPAddress(192, 168, 0, 10), IPAddress(192, 168, 0, 10), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid.c_str(), ap_pass.c_str());
}
// ------------------------------------
void setup_telnet(void){
  server.begin();
  server.setNoDelay(true);
}
// ------------------------------------
void setup_http(void) {
  websvr.on("/", handleTopPage);
  websvr.onNotFound(handleNotFound);
  websvr.begin();
}
// ------------------------------------
void setup() {
  setup_eeprom();
  setup_ram();
  setup_com();
  setup_wifi();
  setup_mDNS();
  setup_telnet();
  setup_http();
}
// ****************************************************************
// * loop
// ****************************************************************
void loop_timer(void){
  //check Sleep Timer
  if ((unsigned long)(millis()-timer_sleep)>SLEEP_TIMER)
  {
    DEBUG_PRINT("deepSleep");
    ESP.deepSleep(0); 
  }
}
// ------------------------------------
void loop_client(void){
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
}
// ------------------------------------
void loop_Telnet2Serial(void){
  uint8_t i;
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i] && serverClients[i].connected()){
      if(serverClients[i].available()){
        timer_sleep = millis();
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
// ------------------------------------
void loop_WebSvr(void){
  websvr.handleClient();
}
// ------------------------------------
void loop() {
  loop_Telnet2Serial();
  loop_client();
  loop_WebSvr();
  loop_timer();
}



