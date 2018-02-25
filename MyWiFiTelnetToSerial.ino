// ****************************************************************
// MyWiFiTelnetToSerial
//  Ver0.01 2018.2.25
//  copyright Sakayanagi
// ****************************************************************

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
// ****************************************************************
// * Const, RAM
// ****************************************************************
const char *HostName = "ESP_OBDII";

//Sleep Timer 30min
#define SLEEP_TIMER (unsigned long)(30*60*1000)
unsigned long time_old = 0;

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1
WiFiServer server(35000);
WiFiClient serverClients[MAX_SRV_CLIENTS];

ESP8266WebServer websvr(80);  //web server

char ssid[33];
char pass[64];
#define SSID_DEFAULT "ESP_OBDII"
#define PASS_DEFAULT ""
char myssid[33];
char mypass[64];

#define EEPROM_NUM (sizeof(ssid)+sizeof(pass)+sizeof(myssid)+sizeof(mypass))

// ****************************************************************
// * Prototype
// ****************************************************************

// ****************************************************************
// * support
// ****************************************************************
bool check_char(char *p, unsigned int num)
{
  unsigned int i;
  for(i=0;i<num;i++)
  {
    if(*p<'-') return(false);
    if(*p>'z') return(false);
    if(*p==0) {
      if(i>0) return(true);
      else return(false);
    }
    p++;
  }
}
void check_ssid(void){
  if(!check_char(  ssid,sizeof(ssid)  )) strcpy(ssid,strcat(SSID_DEFAULT,String(ESP.getChipId(),HEX).c_str()));
  if(!check_char(  pass,sizeof(pass)  )) strcpy(pass  ,"");
  if(!check_char(myssid,sizeof(myssid))) strcpy(myssid,"");
  if(!check_char(mypass,sizeof(mypass))) strcpy(mypass,"");
}

void set_EEPROM(void){
  check_ssid();
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i=0;i<sizeof(ssid)  ; i++)  {*pd = ssid[i];   pd++;}
  for (i=0;i<sizeof(pass)  ; i++)  {*pd = pass[i];   pd++;}
  for (i=0;i<sizeof(myssid); i++)  {*pd = myssid[i]; pd++;}
  for (i=0;i<sizeof(mypass); i++)  {*pd = mypass[i]; pd++;}
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
<H2>AP Setting</H2>\
SSID:<input type='text' name='ssid' maxlength='32' value='**ssid'><br>\
PASS:<input type='text' name='pass' maxlength='63' value='**pass'><br>\
<H2>STA Setting</H2>\
SSID:<input type='text' name='myssid' maxlength='32' value='**myssid'><br>\
PASS:<input type='text' name='mypass' maxlength='63' value='**mypass'><br>\
<br><input type='submit' name='Set_ID' value='Set'>\
<br><input type='submit' name='Reset_ID' value='Reset'>\
</form><hr>\
<H2>Status</H2>\
<table border='1'>\
<tr><th>mode</th><th>SSID    </th><th>PASS    </th><th>IP</th></tr>\
<tr><td>AP  </td><td>**ssid  </td><td>**pass  </td><td>**IP</td></tr>\
<tr><td>STA </td><td>**myssid</td><td>**mypass</td><td>**myIP</td></tr>\
</table>\
</body></html>";

  if (websvr.hasArg("Reset_ID")){ssid[0]=0;pass[0]=0;myssid[0]=0;mypass[0]=0;set_EEPROM();}
  if (websvr.hasArg("Set_ID")) {
    if (websvr.hasArg("ssid")) strcpy(ssid,websvr.arg("ssid").c_str());
    if (websvr.hasArg("pass")) strcpy(pass,websvr.arg("pass").c_str());
    if (websvr.hasArg("myssid")) strcpy(myssid,websvr.arg("myssid").c_str());
    if (websvr.hasArg("mypass")) strcpy(mypass,websvr.arg("mypass").c_str());
    set_EEPROM();
  }

  html.replace("**ssid",ssid);
  html.replace("**pass",pass);
  html.replace("**IP",WiFi.softAPIP().toString());
  html.replace("**myssid",myssid);
  html.replace("**mypass",mypass);
  if(WiFi.status() == WL_CONNECTED) html.replace("**myIP",WiFi.localIP().toString());
  else html.replace("**myIP","Disconnect");
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
}
// ------------------------------------
void setup_eeprom(void) {
  //EEPROM
  EEPROM.begin(EEPROM_NUM);
  unsigned char data[EEPROM_NUM];
  unsigned char *pd = data;
  int i;
  for (i = 0; i < EEPROM_NUM; i++) {data[i] = EEPROM.read(i);}
  for (i=0;i<sizeof(ssid);i++){ssid[i] = *pd; pd++;}
  for (i=0;i<sizeof(pass);i++){pass[i] = *pd; pd++;}
  for (i=0;i<sizeof(myssid);i++){myssid[i] = *pd; pd++;}
  for (i=0;i<sizeof(mypass);i++){mypass[i] = *pd; pd++;}
  check_ssid();
}
// ------------------------------------
void setup_com(void){
  Serial.begin(38400);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(myssid,mypass);
  
  WiFi.softAPConfig(IPAddress(192, 168, 0, 10), IPAddress(192, 168, 0, 10), IPAddress(255, 255, 255, 0));
//  WiFi.softAP(ssid, pass);
  WiFi.softAP(SSID_DEFAULT);

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
void setup_mDNS(void) {
  WiFi.hostname(HostName);
  if (!MDNS.begin(HostName,WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
  }
//  if (!MDNS.begin(HostName.c_str(),WiFi.softAPIP())) {
//    Serial.println("Error setting up MDNS responder!2");
//  }
  Serial.println("mDNS responder started");
  MDNS.addService("telnet", "tcp", 35000);
  MDNS.addService("http", "tcp", 80);
}
// ------------------------------------
void setup() {
  setup_eeprom();
  setup_ram();
  setup_com();
  setup_http();
  setup_mDNS();
}
// ****************************************************************
// * loop
// ****************************************************************
void loop_timer(void){
  //check Sleep Timer
  if ((unsigned long)(millis()-time_old)>SLEEP_TIMER)
  {
    ESP.deepSleep(0); 
  }
}
// ------------------------------------
void loop_AP(void){
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
// ------------------------------------
void loop_WebSvr(void){
  websvr.handleClient();
}
// ------------------------------------
void loop() {
  loop_Telnet2Serial();
  loop_AP();
  loop_WebSvr();
  loop_timer();
}



