#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pixel-millenium5.h"
#include "LatoMedium8.h"
#include "LatoMedium12.h"
#include "digital12.h"
//#include "invader.h"
#include "wemoslogo.h"
#include "wifilogo.h"  

#include "wemos_sht3x.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ArduinoOTA.h>

#include <WiFiUdp.h>
#include <Timezone.h>

#include <Wire.h>

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.google.com";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

WiFiUDP udp;
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "delhoume"
#define AIO_KEY         "5cefb5958805493da7dc5610a4c47eb9"
#define TEMP_FEED       "temperature"
#define HUMI_FEED       "humidity"

struct WeatherInfo {
  float temperature;
  float humidity;
  boolean error;
  char description[16];
  char fulldescription[32];
  char icon[8];
  long time;
};

struct WeatherInfo SHTInfo;
struct WeatherInfo OWMInfo;
struct WeatherInfo NTPInfo;

WiFiClient wifiClient;
Adafruit_MQTT_Client mqtt(&wifiClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish mqttTemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish mqttHum = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

ESP8266WebServer webServer(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81

// openweathermap
HTTPClient owm;

const char* mdnsName = "wemos"; // Domain name for the mDNS responder, connect to http://wemos.local

// degree is 176 (B0)
//fontconvert.exe Lato-Medium.ttf 12 20 255 > LatoMedium12.h

// wemos I2C : D1 = SCL D2 = SDA

SHT3X sht30;

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);


// absolute
inline uint8_t centerVA(uint8_t height) {
  return (display.height() - height) / 2;
}

inline uint8_t centerHA(uint8_t width) {
  return (display.width() - width) / 2;
}

inline void displayWifiLogo() {
  display.drawXBitmap(centerHA(WiFi_width), centerVA(WiFi_height), 
                     WiFi_Logo, WiFi_width, WiFi_height, WHITE);
}

inline void displayText(const char* text, uint16_t xpos, uint16_t ypos) {
  display.setCursor(xpos, ypos);
  display.print(text);
}

void displayTextCenterH(const char* text, uint16_t ypos);
 
void configModeCallback (WiFiManager *myWiFiManager) {
  display.clearDisplay();
  displayWifiLogo();
  char buffer[32];
  sprintf(buffer, "SSID %s", myWiFiManager->getConfigPortalSSID().c_str());
  displayTextCenterH(buffer, 40);
  display.display();
}

const char* owmApiKey    = "95b2ed7cdfc0136948c0a9d499f807eb";
const char* owmArcueilId = "6613168";
const char* owmLang      = "fr"; // fr encodes accents in utf8, not easy to convert

char owmURL[128];

void initSHT() {
  SHTInfo.error = true;
}

void initOWM() {
  sprintf(owmURL, 
	  "http://api.openweathermap.org/data/2.5/weather?id=%s&lang=%s&appid=%s&units=metric", 
	  owmArcueilId, 
	  owmLang, 
	  owmApiKey);
    OWMInfo.error = true;
}

void initNTP() {
  NTPInfo.error = true;
}

/* UTF-8 to ISO-8859-1/ISO-8859-15 mapper.
 * Return 0..255 for valid ISO-8859-15 code points, 256 otherwise.
*/
static inline unsigned int to_latin9(const unsigned int code) {
    /* Code points 0 to U+00FF are the same in both. */
    if (code < 256U)
        return code;
    switch (code) {
    case 0x0152U: return 188U; /* U+0152 = 0xBC: OE ligature */
    case 0x0153U: return 189U; /* U+0153 = 0xBD: oe ligature */
    case 0x0160U: return 166U; /* U+0160 = 0xA6: S with caron */
    case 0x0161U: return 168U; /* U+0161 = 0xA8: s with caron */
    case 0x0178U: return 190U; /* U+0178 = 0xBE: Y with diaresis */
    case 0x017DU: return 180U; /* U+017D = 0xB4: Z with caron */
    case 0x017EU: return 184U; /* U+017E = 0xB8: z with caron */
    case 0x20ACU: return 164U; /* U+20AC = 0xA4: Euro */
    default:      return 256U;
    }
}

/* Convert an UTF-8 string to ISO-8859-15.
 * All invalid sequences are ignored.
 * Note: output == input is allowed,
 * but   input < output < input + length
 * is not.
 * Output has to have room for (length+1) chars, including the trailing NUL byte.
*/
size_t utf8_to_latin9(char *const output, const char *const input, const size_t length) {
    unsigned char             *out = (unsigned char *)output;
    const unsigned char       *in  = (const unsigned char *)input;
    const unsigned char *const end = (const unsigned char *)input + length;
    unsigned int               c;

    while (in < end)
        if (*in < 128)
            *(out++) = *(in++); /* Valid codepoint */
        else
        if (*in < 192)
            in++;               /* 10000000 .. 10111111 are invalid */
        else
        if (*in < 224) {        /* 110xxxxx 10xxxxxx */
            if (in + 1 >= end)
                break;
            if ((in[1] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x1FU)) << 6U)
                             |  ((unsigned int)(in[1] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 2;

        } else
        if (*in < 240) {        /* 1110xxxx 10xxxxxx 10xxxxxx */
            if (in + 2 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x0FU)) << 12U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[2] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 3;

        } else
        if (*in < 248) {        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 3 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x07U)) << 18U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[3] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 4;

        } else
        if (*in < 252) {        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 4 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x03U)) << 24U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[4] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 5;

        } else
        if (*in < 254) {        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
            if (in + 5 >= end)
                break;
            if ((in[1] & 192U) == 128U &&
                (in[2] & 192U) == 128U &&
                (in[3] & 192U) == 128U &&
                (in[4] & 192U) == 128U &&
                (in[5] & 192U) == 128U) {
                c = to_latin9( (((unsigned int)(in[0] & 0x01U)) << 30U)
                             | (((unsigned int)(in[1] & 0x3FU)) << 24U)
                             | (((unsigned int)(in[2] & 0x3FU)) << 18U)
                             | (((unsigned int)(in[3] & 0x3FU)) << 12U)
                             | (((unsigned int)(in[4] & 0x3FU)) << 6U)
                             |  ((unsigned int)(in[5] & 0x3FU)) );
                if (c < 256)
                    *(out++) = c;
            }
            in += 6;

        } else
            in++;               /* 11111110 and 11111111 are invalid */

    /* Terminate the output string. */
    *out = '\0';
    return (size_t)(out - (unsigned char *)output);
}

void displayHexString(const char* string) {
  int len = strlen(string);
  for (int idx = 0; idx < len; ++idx) {
    Serial.print(string[idx], HEX);
    Serial.print(" ");
    Serial.print(string[idx]);
    Serial.print(" ");
  }
  Serial.println();
}

void getOWMInfo() {
  OWMInfo.error = true;
  if (WiFi.isConnected()) {
    owm.begin(owmURL);
    if (owm.GET()) {
      String json = owm.getString();
//      Serial.println(json);
//     Serial.println(json.length());
      DynamicJsonBuffer jsonBuffer(1024);
      JsonObject& root = jsonBuffer.parseObject(json);
      if (root.success()) {
        float temp = root["main"]["temp"];
        OWMInfo.temperature = round(temp);
        JsonObject& weather = root["weather"][0];
        strcpy(OWMInfo.description, weather["main"]);
        static char utf8buf[64];
        strcpy(utf8buf, weather["description"]);
         utf8_to_latin9(OWMInfo.fulldescription, utf8buf, strlen(utf8buf));
 //     displayHexString(OWMInfo.fulldescription);
        strcpy(OWMInfo.icon, weather["icon"]);

 //       OWMInfo.time = atol(root["dt"]);
 //      Serial.println(OWMInfo.temperature);
 //      Serial.println(OWMInfo.icon);
	      OWMInfo.error = false;
      }
    }
    owm.end();
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * /* payload */, size_t /* length */);

void initOTA() {
      ArduinoOTA.setHostname("WemosSHT");
    ArduinoOTA.setPassword("wemos");
    ArduinoOTA.onStart([]() {});
    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      display.setFont(&Lato_Medium12pt8b);
      display.clearDisplay();
      unsigned int realprogress = (progress / (total / 100));
      char buffer[16];
      sprintf(buffer, "OTA: %.2d %%", realprogress);
      displayTextCenter(buffer);
      display.display();
    });
    ArduinoOTA.onError([](ota_error_t error) {});
    ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);
  //sht3xd.begin(0x45); // I2C address: 0x44 or 0x45
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64) - 64x48 for wemos oled
  Wire.setClock(400000);

  display.setRotation(2); // 180 degrees
   // show splash screen
   display.clearDisplay();
   display.drawBitmap(0, 0, wemos_logo_64x48_normal, 64, 48, WHITE);
  display.display();
  display.setTextColor(WHITE, BLACK);

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
 // wifiManager.setDebugOutput(true);
  wifiManager.setMinimumSignalQuality(30);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("wemos"); //  no password

  initSHT();
  initOWM();
  initNTP();

  if (WiFi.isConnected()) {
    udp.begin(localPort);

    initOTA();
    // start file system
    SPIFFS.begin(); 
    // start webSockets
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    // start mDNS
    MDNS.begin(mdnsName);
    // start webserver
    webServer.onNotFound(handleNotFound);
    webServer.begin();

    getOWMInfo();

    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP); 

  }
  updateSHT();
}

String getContentType(String filename){
  if(filename.endsWith(".html")) 
    return "text/html";
  else if(filename.endsWith(".css")) 
    return "text/css";
  else if(filename.endsWith(".js")) 
    return "application/javascript";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
//  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
   if (SPIFFS.exists(path)) {                            // If the file exists
      String contentType = getContentType(path);            // Get the MIME type
      File file = SPIFFS.open(path, "r");                 // Open it
      webServer.streamFile(file, contentType); // And send it to the client
      file.close();                                       // Then close the file again
      return true;
  }
  return false;                                         // If the file doesn't exist, return false
}

void handleNotFound() {
  if (!handleFileRead(webServer.uri()))
    webServer.send(404, "text/plain", "404: File Not Found");
}

// num is client id
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * /* payload */, size_t /* length */) {
  switch (type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED:
      break;
    case WStype_TEXT: // always send back JSON string
      sendWebSocketValues(num);
      break;
    default:
    break;
  }
}

//  could use ArduinoJson
const char* json = "{\"t\":\"%.1f\",\"h\":\"%.1f\"}";
    
void sendWebSocketValues(uint8_t num) {
       // send to websockets as JSON string
       // send nothing if error
      if (SHTInfo.error ==  false) {
        static char buffer[32];
        sprintf(buffer, json, SHTInfo.temperature, SHTInfo.humidity);
//        Serial.println(buffer);
        webSocket.sendTXT(num, buffer);
      } 
    } 

uint8_t rssiToQuality(long rssi) {
     // -120 to 0 DBm
   uint8_t quality;
   if(rssi <= -100)
     quality = 0;
   else if(rssi >= -50)
     quality = 100;
   else
     quality = 2 * (rssi + 100);
  return quality;
}


void displayConnectionStatus(uint16_t startx, uint16_t starty) {
    if (WiFi.isConnected()) {
      uint8_t quality = rssiToQuality(WiFi.RSSI());
      uint16_t offset = 4;
      if (quality >= 10)
        display.fillRect(startx, starty - 3, 3, 3, WHITE);
      else
        display.drawRect(startx, starty - 3, 3, 3, WHITE);
      startx += offset;
      if (quality >= 25)
        display.fillRect(startx, starty - 5, 3, 5, WHITE);
      else
        display.drawRect(startx, starty - 5, 3, 5, WHITE);
      startx += offset;
       if (quality >= 50)
        display.fillRect(startx, starty - 7, 3, 7, WHITE);
      else
        display.drawRect(startx, starty - 7, 3, 7, WHITE);
      startx += offset;
       if (quality >= 80)
        display.fillRect(startx, starty - 9, 3, 9, WHITE);
      else
        display.drawRect(startx, starty -9, 3, 9, WHITE);
      }
 }

unsigned long lastUpdateTime = 0;
unsigned long lastDisplayTime = 0;

void displayContents();

void loop() {
  // is it necessary when only publishing data ?
    webSocket.loop();
    webServer.handleClient();
    ArduinoOTA.handle();

    unsigned long currentTime = millis();
    
    if ((currentTime - lastDisplayTime) >= 3000) { 
       displayContents();
      lastDisplayTime = currentTime;
    }
    if ((currentTime - lastUpdateTime) >= 30000) { 
      updateSHT();
     sendMQTTValues();
      getOWMInfo();
      sendNTPpacket(timeServerIP); 
     lastUpdateTime = currentTime;
   }
   checkNTP();      
  }


void checkNTP() {
  if (udp.parsePacket() != 0) { 
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    uint32_t NTPTime = (packetBuffer[40] << 24) | (packetBuffer[41] << 16) | (packetBuffer[42] << 8) | packetBuffer[43];
    const uint32_t seventyYears = 2208988800UL;
    // subtract seventy years:
    uint32_t epoch = NTPTime - seventyYears; 
    Serial.print("Epoch "); Serial.println(epoch);
    NTPInfo.time = epoch;
    NTPInfo.error = false;
  } else {
 //   Serial.println("No NTP");
  }
}


void sendNTPpacket(IPAddress& address) {
  if (WiFi.isConnected()) {
    Serial.println("Requesting NTP");
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
  }
}

void updateSHT() {
  SHTInfo.error = true;
  if (sht30.get() == 0) {
    // bad temp read when sensor is not isolated, CALIBRATION !!
    // nearest 0.5
    float tempCalibration = 0.0; // -7.0
    float humCalibration = 0.0;  // +10.0
    SHTInfo.temperature = round((sht30.cTemp + tempCalibration) * 2.0) / 2.0;
    SHTInfo.humidity = round(sht30.humidity  + humCalibration);
    SHTInfo.error = false;
  }
}

void sendMQTTValues() {
 if (SHTInfo.error == false) {
    if (!mqtt.connected()) {
      mqtt.connect();
    }
     if (mqtt.connected()) {
        mqttTemp.publish(SHTInfo.temperature);
        mqttHum.publish(SHTInfo.humidity);
      }
   }
}

// center in page area

int left = 0;
int top = 0;
int header = 10;

int centerH(int width) {
  return ((64 - width) / 2) + left;
}

// text, starts at bottom
int centerVT(int height) {
  return ((48 - height - header) / 2) + top + height + header; 
}

// image
int centerV(int height) {
  return ((48 - height - header) / 2) + top + header; 
}

void displayTextCenterH(const char* str, int ypos) {
    int16_t x1, y1;
    uint16_t w, h; 
    display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    displayText(str, centerH(w), ypos);
}

void displayTextCenter(const char* str) {
    int16_t x1, y1;
    uint16_t w, h; 
    display.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    displayText(str, centerH(w), centerVT(h));
}


void displayOWMTemp() {
  display.setFont(&Lato_Medium12pt8b);
  if (OWMInfo.error == false) {
    displayTemp(OWMInfo.temperature);
  } else {
    displayTextCenter("?");
  }
}

void displayTemp(float temp) {
    static char buffer[10];
    char* buf = buffer;
    if (temp < 0) {
      buffer[0] = '-';
      buf++;
    }
    dtostrf(fabs(round(temp)), 2, 0, buf);
//    buf[2] = ' ';
    buf[2] = 176;
    buf[3] = 0;
    displayTextCenter(buffer);
}

struct OWMIcon {
  const char* names; // comma separated
  uint8_t width;
  uint8_t height;
  const uint8_t* data;
};

//#include "weathericons.h"
#include "weathericonic.h"

struct OWMIcon* findIcon(const char* name) {
  int numIcons = sizeof(icons_weather_iconic) / sizeof(struct OWMIcon);
  for (int idx = 0; idx < numIcons; ++idx) {
    struct OWMIcon* current = &icons_weather_iconic[idx];
    char* names = strdup(current->names);
    char* tofree = names; // names will be modified
    char* found;
     while((found = strsep(&names, ",")) != NULL) {
      if (!strcmp(name, found)) {
        free(tofree);
         return current;
      }
    }
    free(tofree);
  }
  return NULL;
}

void displayOWMIcon() {
  if (OWMInfo.error == false) {
    display.setFont(&pixel_millennium_regular5pt8b);
    struct OWMIcon* icon = findIcon(OWMInfo.icon);
    if (icon != NULL) {
      display.drawBitmap(centerH(icon->width), centerV(icon->height) - 2, icon->data, icon->width, icon->height, WHITE);
      displayTextCenterH(OWMInfo.fulldescription, 44);
     } else { 
      displayTextCenter(OWMInfo.icon);
    }
  } else {
    displayTextCenter("?");
  }
}

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;

void displayNTPTime() {
   display.setFont(&DS_DIGIB12pt7b);
  if (NTPInfo.error == false) {
   time_t utc = NTPInfo.time;
   time_t local = CE.toLocal(utc, &tcr);
   char buffer[32];
   sprintf(buffer, "%.2d:%.2d", hour(local), minute(local));
   displayTextCenter(buffer);
  } else {
    displayTextCenter("??:??");
  }
}

const char* days[] = { "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"  };
//const char* monthes[] = { "Janvier", "F�vrier", "Mars", "Avril", "Mai", "Juin", "Juillet", 
//                          "Ao�t", "Septembre", "Octobre", "Novembre", "D�cembre" };
  
void displayNTPDate() {
   display.setFont(&Lato_Medium8pt8b);
   if (NTPInfo.error == false) {
     time_t utc = NTPInfo.time;
     time_t local = CE.toLocal(utc, &tcr);
     char buffer[32];
     sprintf(buffer, "%s %d", days[weekday(local) - 1], day(local)); // , monthes[month(local) -1]);
     displayTextCenter(buffer);
  } else {
    displayTextCenter("?");
  }
}

const uint8_t home_width = 8;
const uint8_t home_height = 8;
const uint8_t home_data[] PROGMEM = {
  B00010000,
  B00101000,
  B01000100,
  B10000010,
  B10000010,
  B10000010,
  B10000010,
  B11111110
};

// TODO: find a nice logo 8x8
void displayHomeLogo() {
  display.drawXBitmap(2, 12, home_data, home_width, home_height, WHITE);
}

void displaySHTTemp() {
  display.setFont(&Lato_Medium12pt8b);
 if (SHTInfo.error == false) {
    displayTemp(SHTInfo.temperature);
  } else {
    displayTextCenter("?");
  }
  displayHomeLogo();
}

void displaySHTHum() {
 display.setFont(&Lato_Medium12pt8b);
 if (SHTInfo.error == false) {
   static char buffer[10];
   dtostrf(SHTInfo.humidity, 2, 0, buffer);
 //  buffer[2] = ' ';
   buffer[2]= '%';
   buffer[3] = 0;
   displayTextCenter(buffer);
 } else {
   displayTextCenter("?");
 }
  displayHomeLogo();
}

void displayMQTTStatus() {
 if (mqtt.connected()) {
   display.setFont(&pixel_millennium_regular5pt8b);
   displayText("mqtt", 5, 7);
 }
}

typedef void (*DISPLAYFUN)();

DISPLAYFUN displayFuns[] = {
  displaySHTTemp,
  displaySHTHum,
  displayOWMTemp,
  displayOWMIcon,
  displayNTPTime,
  displayNTPDate
};

uint8_t currentPage = 0;

void displayContents() {
  display.clearDisplay();
  // englobing rectangle
  //display.drawRect(0, 0, 64, 48, WHITE);
  //display.drawFastHLine(0, 10, 64, WHITE);
  uint8_t numPages = sizeof(displayFuns) / sizeof(DISPLAYFUN);
  // call function
  displayFuns[currentPage]();
  currentPage = (currentPage + 1) % numPages;
  displayConnectionStatus(40, 10);
  displayMQTTStatus(); 
  display.display();
}
