//#include "Zone.cpp"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <TimeLib.h>
#include <SimpleTimer.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include "Elasticsearch.h"
#include "sprinkler_controller.h"

#define BUILT_IN_LED 2
#define REVERSE_POLARITY_PIN 12
#define BOOSTER_PIN 13

#define SP_0_PIN 26
#define SP_1_PIN 27
#define SP_2_PIN 14
#define SP_3_PIN 25

const int timeZone = 0;     // Central European Time
const char* ssid = "Burton";
const char* password = "Takeachance01";
const String syncUrl = "http://192.168.1.7:32080/david.cuerrier/irrigation-system/-/raw/master/home.html";


//Zone Zone1 = Zone(1, SP_1_PIN);

WebServer server(80);
HTTPClient syncHttp;

ElasticsearchClient esc("http://192.168.1.7:9200", "sprinkler_log");
WiFiUDP udp;
IPAddress timeServer(209, 115, 181, 113);
String webHomeContent = "No Content";
SimpleTimer timer;

void setup() {

  // Define each PIN for its own functionality
  pinMode(      BUILT_IN_LED,           OUTPUT);
  pinMode(      REVERSE_POLARITY_PIN,   OUTPUT);
  pinMode(      BOOSTER_PIN,            OUTPUT);
  pinMode(      SP_0_PIN,               OUTPUT);
  pinMode(      SP_1_PIN,               OUTPUT);
  pinMode(      SP_2_PIN,               OUTPUT);
  pinMode(      SP_3_PIN,               OUTPUT);


  // Initialisez PINs to GND
  digitalWrite( REVERSE_POLARITY_PIN,   LOW);
  digitalWrite( BOOSTER_PIN,            LOW);
  digitalWrite( SP_0_PIN,               LOW);
  digitalWrite( SP_1_PIN,               LOW);
  digitalWrite( SP_2_PIN,               LOW);
  digitalWrite( SP_3_PIN,               LOW);


  // Initiate WiFi connectivity
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(1000);
    ESP.restart();
  }

  // Initiate Over The Air update server
  ArduinoOTA.begin();

  // Sync local time with NTP server
  setSyncProvider(getNtpTime);

  // Define webserver paths
  initWebserver();

  writeLog("MAIN", "ESP just started up! You may reach it at: http://" + WiFi.localIP().toString() + "/");
  writeLog("MAIN", "Wifi signal is: " + String(WiFi.RSSI()));

  // Update local web interface from the local gitlab
  cloneWebInterface();
  timer.setInterval(15000L, sendRecuringMetric);
}

void loop() {

  timer.run();

  if (WiFi.status() != WL_CONNECTED)
    ESP.restart();

  ArduinoOTA.handle();
  server.handleClient();
  delay(500); 
}

void sendRecuringMetric() {
  //writeMetric("MAIN", "uptime", millis());
  //writeMertic("ZONE", "state", Zone1.isRunning, 1);
  //writeMertic("ZONE", "state", Zone2.isRunning, 2);
}

void toogleSp(int pin, bool state) {
  
    // Turn on reverser if needed
    if (!state) {
      digitalWrite(REVERSE_POLARITY_PIN, !state);
      delay(500);
    }
  
    // Prime the solenoid
    digitalWrite(pin, HIGH);
    delay(1000);
    digitalWrite(pin, LOW);
  
    for (int i = 0; i <= 1; i++) {
      // Charge the booster !!
      digitalWrite(BOOSTER_PIN, HIGH);
      delay(3000);
      digitalWrite(BOOSTER_PIN, LOW);
  
      // Strike the solenoid and wait 1.5 sec
      digitalWrite(pin, HIGH);
      delay(1500);
      // Stop striking the solenoid witch will be left in its position
      digitalWrite(pin, LOW);
    }
  
    delay(500); // Wait a bit befor turning back the polarity to avoid backfire in the solenoid
    digitalWrite(REVERSE_POLARITY_PIN, LOW);
  }

void initWebserver() {

  server.on("/", []() {

    String m = String(millis());

    String content = webHomeContent;
    content.replace("{UPTIME}", m);

    server.send(200, "text/html", content);
  });

  server.on("/sync-web", []() {
    cloneWebInterface();
  });

  // /timer/{zone}/{min}
  server.on("/timer/{}/{}", HTTP_POST, []() {
    String zone = server.pathArg(0);
    String timeout = server.pathArg(1);

    writeLog("TIMER", "Turn on zone " + zone + " with a timeout of " + timeout + " millis");
    
    switch (zone.toInt()) {

      case 0:
        server.send(501, "application/json", "{\"reason\":\"The controller head #" + zone + " has not been implemented!\"}");
        break;

      case 1:
        toogleSp(SP_1_PIN, true);
        //timer.setTimeout(timeout.toInt(), stopZone1);
        break;

      case 2:
        toogleSp(SP_2_PIN, true);
        //timer.setTimeout(timeout.toInt(), stopZone2);
        break;

      case 3:
        server.send(501, "application/json", "{\"reason\":\"The controller head #" + zone + " has not been implemented!\"}");
        break;

      default:
        server.send(404, "application/json", "{\"reason\":\"The controller head #" + zone + " does not exist!\"}");
        break;
    }

    server.send(200, "application/json", "{\"zone\":\"" + zone + "\", \"pin\":\"" +  + "\"}");

  });

  server.on("/trigger/{}/{}", []() {

    String sp = server.pathArg(0);
    String state = server.pathArg(1);

    state.toUpperCase();

    bool bstate = (state == "TRUE");

    switch (sp.toInt()) {

      case 0:
        server.send(501, "application/json", "{\"reason\":\"The controller head #" + sp + " has not been implemented!\"}");
        break;

      case 1:
        toogleSp(SP_1_PIN, bstate);
        break;

      case 2:
        toogleSp(SP_2_PIN, bstate);
        break;

      case 3:
        server.send(501, "application/json", "{\"reason\":\"The controller head #" + sp + " has not been implemented!\"}");
        break;

      default:
        server.send(404, "application/json", "{\"reason\":\"The controller head #" + sp + " does not exist!\"}");
        break;
    }

    server.send(200, "application/json", "{\"sp\":\"" + sp + "\", \"pin\":\"" +  + "\"}");
  });

  server.begin();
}



void cloneWebInterface() {
  syncHttp.begin(syncUrl);
  int httpCode = syncHttp.GET();
  
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      webHomeContent = syncHttp.getString();
      server.sendHeader("Location", "/");
      server.send(303);
  
    } else {
      server.send(500, "text/html", "ERROR");
    }
  
  } else {
    server.send(500, "text/html", "ERROR");
  }
  
  syncHttp.end();
}




void writeLog(String feature, String message) {
  esc.sendLog(String(now(), DEC), feature, message);
}
void writeMetric(String feature, String metric, float value, int zone = -1) {
  esc.sendMetric(String(now(), DEC), feature, metric, zone, value, "sprinkler_metric");
}
void writeMetric(String feature, String metric, String value, int zone = -1) {
  esc.sendMetric(String(now(), DEC), feature, metric, zone, value, "sprinkler_metric");
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precixsasion
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
