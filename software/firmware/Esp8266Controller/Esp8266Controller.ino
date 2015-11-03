#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include "FS.h"

/* Settings you may want to configure for your Scob */
#define SCOB_SSID "Scob"
#define SCOB_PWD ""
#define SCOB_URL "sc.ob"
static const IPAddress apIP(192, 168, 4, 1);
static const IPAddress mask(255, 255, 255, 0);

/* Settings you should leave alone */
#define DNS_PORT 53
#define MAX_PARAM_LENGTH 50
#define led 2
#define RESPONSE_TIME 200

DNSServer dnsServer;
ESP8266WebServer server(80);

IPAddress localIP(0,0,0,0);

static void urldecode2(char *dst, const char *src)
{
  char a, b;
  while (*src) {
    if ((*src == '%') &&
      ((a = src[1]) && (b = src[2])) &&
      (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a')
        a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else {
        *dst++ = *src++;
    }
  }
  *dst++ = '\0';
}

static void handleCommand()
{
  digitalWrite(led, 1);

  if (server.hasArg("action")) {
    // Parse args
    String cmd = server.arg("action");
    char arg[MAX_PARAM_LENGTH];
    char temp[MAX_PARAM_LENGTH];
    if (server.hasArg("p1")) {
      server.arg("p1").toCharArray(arg, MAX_PARAM_LENGTH);
      urldecode2(temp, arg);
      cmd += " " + String(temp);
    }
    if (server.hasArg("p2")) {
      server.arg("p2").toCharArray(arg, MAX_PARAM_LENGTH);
      urldecode2(temp, arg);
      cmd += " " + String(temp);
    }

    // Clear input in anticipation of a response.
    while(Serial.available())
      Serial.read();

    // Send command
    Serial.println(cmd);

    // Wait for a limited time for a reply
    Serial.setTimeout(RESPONSE_TIME);
    String response = Serial.readStringUntil('\n');

    server.send(200, F("text/plain"), response);

  } else {
    server.send(200, F("text/plain"), F("err"));
  }

  digitalWrite(led, 0);
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

static void handleNotFound()
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
  digitalWrite(led, 0);
}

void setup(void){
  Serial.begin(9600);
  Serial.println();
  Serial.println(F("Scbob Wifi Interface"));

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  SPIFFS.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, mask);
  if (SCOB_PWD == "")
    WiFi.softAP(SCOB_SSID);
  else
    WiFi.softAP(SCOB_SSID, SCOB_PWD);

  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::NonExistentDomain);
  dnsServer.start(DNS_PORT, SCOB_URL, apIP);

  // Most frequently accesed pages first for speed.
  server.on("/cmd", handleCommand);
  server.onNotFound([](){
    if (!handleFileRead(server.uri()))
      handleNotFound();
  });

  server.begin();
}

void loop(void){
  dnsServer.processNextRequest();
  server.handleClient();

  // write IP address when connected!
  if (WiFi.status() == WL_CONNECTED && (uint32_t)localIP == 0) {
    localIP = WiFi.localIP();
    Serial.print("WT ");
    Serial.println(WiFi.localIP());
  }
}
