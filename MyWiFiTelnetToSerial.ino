// ****************************************************************
// MyWiFiTelnetToSerial
//  Ver0.01 2018.2.25
//  copyright Sakayanagi
// ****************************************************************

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
// ****************************************************************
// * Const, RAM
// ****************************************************************

//Sleep Timer 30蛻�縺ｫ險ｭ螳�
#define SLEEP_TIMER (unsigned long)(30*60*1000)
unsigned long time_old = 0;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1
const char *ssid = "ESP_OBDII";
const char *pass = "";
WiFiServer server(35000);
WiFiClient serverClients[MAX_SRV_CLIENTS];

ESP8266WebServer websvr(80);  //web繧ｵ繝ｼ繝舌�ｼ

char myssid[33];
char mypass[64];
#define EEPROM_NUM (sizeof(myssid)+sizeof(mypass))

// ****************************************************************
// * Prototype
// ****************************************************************

// ****************************************************************
// * HomePage
// ****************************************************************
void handleTopPage() {
  String html = "\
<!DOCTYPE html><html><head><title>Setting</title></head><body>\
<H1>ESP_OBDII Setting</H1>\
<form name='inputform' action='' method='POST'>\
<H2>WiFi Setting</H2>\
SSID:<input type='text' name='ssid' maxlength='32' value='**myssid'><br>\
PASS:<input type='text' name='pass' maxlength='63' value='**mypass'><br>\
<br><input type='submit' name='SUBMIT' value='Submit'>\
</form></body></html>\
<H2>Connection Status</H2>\
<H3>AP<dl><dt>ssid</dt><dd>**ssid</dd><dt>pass</dt><dd>**pass</dd></dl>\
<H3>STA <dl><dt>ssid</dt><dd>**myssid</dd><dt>pass</dt><dd>**mypass</dd><dt>IP</dt><dd>**IP</dd></dl>\
";

  if (websvr.hasArg("ssid")) strcpy(myssid,websvr.arg("ssid").c_str());
  if (websvr.hasArg("pass")) strcpy(mypass,websvr.arg("pass").c_str());
  if (websvr.hasArg("SUBMIT")) {
    unsigned char data[EEPROM_NUM];
    unsigned char *pd = data;
    int i;
    for (i=0;i<sizeof(myssid); i++)  {*pd = myssid[i]; pd++;}
    for (i=0;i<sizeof(mypass); i++)  {*pd = mypass[i]; pd++;}
    for (i = 0; i < EEPROM_NUM; i++) {EEPROM.write(i, data[i]);}
    EEPROM.commit();
  }

  html.replace("**ssid",ssid);
  html.replace("**pass",pass);
  html.replace("**myssid",myssid);
  html.replace("**mypass",mypass);
  if(WiFi.status() == WL_CONNECTED) html.replace("**IP",WiFi.localIP().toString());
  else html.replace("**IP","Disconnect");
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
void setup_eeprom(void) {
  //EEPROM
  EEPROM.begin(EEPROM_NUM);
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i = 0; i < EEPROM_NUM; i++) {data[i] = EEPROM.read(i);}
  for (i=0;i<sizeof(myssid);i++){myssid[i] = *pd; pd++;}
  if (strlen(myssid)==0){strcpy(myssid,"myssid");}
  for (i=0;i<sizeof(mypass);i++){mypass[i] = *pd; pd++;}
}

void setup_com(void){
  Serial.begin(38400);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(myssid,mypass);
  
  WiFi.softAPConfig(IPAddress(192, 168, 0, 10), IPAddress(192, 168, 0, 10), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, pass);

  server.begin();
  server.setNoDelay(true);
}

// ------------------------------------
void setup_http(void) {
  websvr.on("/", handleTopPage);
  websvr.onNotFound(handleNotFound);
  websvr.begin();
}

void setup() {
  setup_eeprom();
  setup_com();
  setup_http();
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

  websvr.handleClient();
}


