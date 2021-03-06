#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "index.html.h"
#include "main.js.h"
#include <WS2812FX.h>
 
 
#define WIFI_SSID "Unio Labs Ltd"
#define WIFI_PASSWORD "Mah00941888"
#define WIFI_TIMEOUT 30000              // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80
#define DEFAULT_COLOR 0xFF5900
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_SPEED 200
#define DEFAULT_MODE FX_MODE_STATIC
#define BRIGHTNESS_STEP 15              // in/decrease brightness by this amount per click
#define SPEED_STEP 10                   // in/decrease brightness by this amount per click
#define pirPin1 D1 // Input for HC-SR501
#define pirPin2 D2
 
 
// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define LED_PIN 2                       // 0 = GPIO0, 2=GPIO2
 
const char *softAP_ssid = "ESP_ap";
const char *softAP_password = "12345678";
 
/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "esp8266";
 
/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
 
WS2812FX ws2812fx = WS2812FX(0, 2, NEO_GRB + NEO_KHZ800);
 
// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;
 
// Web server
ESP8266WebServer server(80);
 
/* Soft AP network parameters */
IPAddress apIP(192, 168, 1, 1);
IPAddress netMsk(255, 255, 255, 0);
 
 
/** Should I connect to WLAN asap? */
boolean connect;
 
/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;
/** Current WLAN status */
int status = WL_IDLE_STATUS;
String modes = "";
void modes_setup();
int LED_COUNT =0;
String c;
int LED_STEP =0;
String g;
int flagLDR =0;
String j;
 
int pirValue1; // variable to store read PIR Value
int pirValue2;
 
unsigned long last_wifi_check_time = 0;
 
//WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
//ESP8266WebServer server = ESP8266WebServer(HTTP_PORT);
 
 
void setup(){
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");
 
  modes.reserve(5000);
  modes_setup();
 
  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();
 
  Serial.println("Wifi setup");
  //wifi_setup();
 
  Serial.println("HTTP server setup");
  // server.on("/", srv_handle_index_html);
  // server.on("/main.js", srv_handle_main_js);
  // server.on("/modes", srv_handle_modes);
  // server.on("/set", srv_handle_set);
  // server.onNotFound(srv_handle_not_found);
 
  Serial.println("HTTP server started.");
 
  Serial.println("ready!");
 
  pinMode(pirPin1, INPUT);
  pinMode(pirPin2, INPUT);
 
  delay(1000);
 // Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
 
 
  /* Setup the DNS server redirecting all the domains to the apIP */  
 
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
//  server.onNotFound([]() {
//  server.send_P(200,"text/html", index_html);
//  server.send_P(200, "application/javascript", main_js);
//  server.send(200, "text/plain", modes);
// server.onNotFound([]() {
  //   server.send(200, "text/html", index_html);
  // });
  //server.begin();
  //delay(2500);
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/set", srv_handle_set);
 
 // });
  server.begin();
  Serial.println("HTTP server started");
//  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
 
}
 
 
void loop() {
  unsigned long now = millis();
 
  server.handleClient();
  ws2812fx.service();
 
 if ( digitalRead(pirPin1))
 {
  if (flagLDR == 1)
  {
    float sensorValue = analogRead(A0);
    float voltage = sensorValue * (5.0 / 1023.0);
    if (voltage < 2)
    {
      ws2812fx.setMode(2 % ws2812fx.getModeCount());
    }    
  }
  }
 
  else if ( digitalRead(pirPin1) == LOW)
  {
    if (flagLDR == 1)
    {
      float sensorValue = analogRead(A0);
      float voltage = sensorValue * (5.0 / 1023.0);
      if (voltage < 2)
      {
        ws2812fx.setMode(2 % ws2812fx.getModeCount());
      }
    }
  }  
Serial.println(j);
 
if (connect) {
    Serial.println ( "Connect requested" );
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 60000) ) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print ( "Status: " );
      Serial.println ( s );
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println ( "" );
        Serial.print ( "Connected to " );
        Serial.println ( ssid );
        Serial.print ( "IP address: " );
        Serial.println ( WiFi.localIP() );
 
        // Setup MDNS responder
        if (!MDNS.begin(myHostname)) {
          Serial.println("Error setting up MDNS responder!");
        } else {
          Serial.println("mDNS responder started");
          // Add service to MDNS-SD
          MDNS.addService("http", "tcp", 80);
        }
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
  }
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
 
}
 /* Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
 
 
/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  int a[5] = {0,2,13,14,30};
  //for(uint8_t i=0; i < ws2812fx.getModeCount(); i++) {
  for(int i=0;i<5;i++){
    modes += "<li><a href='#' class='m' id='";
    modes += a[i];
    modes += "'>";
    modes += ws2812fx.getModeName(a[i]);
    modes += "</a></li>";
  }
}
void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  #ifdef STATIC_IP  
    WiFi.config(ip, gateway, subnet);
  #endif
 
  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
 
    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.reset();
    }
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
 
/* #####################################################
#  Webserver Functions
##################################################### */
 
void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}
 
void srv_handle_index_html() {
  Serial.println("Index");
  server.send_P(200,"text/html", index_html);
}
 
void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}
 
void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}
 
void srv_handle_set() {
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(&server.arg(i)[0], NULL, 16);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
      }
    }
 
    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
    }
 
    if(server.argName(i) == "b") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.decreaseBrightness(BRIGHTNESS_STEP);
      } else {
        ws2812fx.increaseBrightness(BRIGHTNESS_STEP);
      }
    }
 
    if(server.argName(i) == "s") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.decreaseSpeed(SPEED_STEP);
      } else {
        ws2812fx.increaseSpeed(SPEED_STEP);
      }
    }
 
        if(server.argName(i) == "LED_COUNT") {
           c = server.arg(i);
    }
 
         if(server.argName(i) == "LED_STEP") {
           g = server.arg(i);
    }
 
          if(server.argName(i) == "flagLDR") {
           j = server.arg(i);
    }
  }
  server.send(200, "text/plain", "OK");
}
 
void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
}
