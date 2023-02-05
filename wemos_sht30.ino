#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306_wemos.h>

// fontconvert.exe fonts/DigitalDisplay.ttf 12 > DigitalDisplay12.h
//#include "DigitalDisplay12.h"
#include "digital12.h"
#include "pixel-millenium5.h"
#include "LatoMedium8.h"
#include "LatoMedium12.h"
#include "wemoslogo.h"
#include "wifilogo.h" 

#include "secrets.h"


struct OWMIcon {
  const char* names; // comma separated
  uint8_t width;
  uint8_t height;
  const uint8_t* data;
};

 //#include "weathericons.h"
#include "weathericonic.h"

#include "wemos_sht3x.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h> 
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include <WiFiUdp.h>
#include <Timezone.h>

#include <Wire.h>

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define TEMP_FEED       "temperature"
#define HUMI_FEED       "humidity"

struct WeatherInfo {
  float temperature;
  float humidity;
  boolean error;
  char description[16]= {0};
  char fulldescription[32] = {0};
  char icon[8] = {0};
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

// openweathermap
HTTPClient owm;
WiFiClient wclient;

// degree is 176 (B0)
//fontconvert.exe Lato-Medium.ttf 12 20 255 > LatoMedium12.h

// wemos I2C : D1 = SCL D2 = SDA

SHT3X sht30;

#define SCREEN_WIDTH 64 // OLED display width, in pixels
#define SCREEN_HEIGHT 48 // OLED display height, in pixels
#define OLED_RESET 0  // GPIO0
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

// forward declarations
void displayTextCenterH(const char* text, uint16_t ypos);
void displayTextCenter(const char* str);
void updateSHT();
void sendMQTTValues();

 
void configModeCallback (WiFiManager *myWiFiManager) {
  display.clearDisplay();
  displayWifiLogo();
  char buffer[32];
  sprintf(buffer, "SSID %s", myWiFiManager->getConfigPortalSSID().c_str());
  display.setFont(&pixel_millennium_regular5pt8b);
  displayTextCenterH(buffer, 40);
  display.display();
}


const char* owmArcueilId = "6613168";
const char* owmLang      = "fr"; // fr encodes accents in utf8, not easy to convert

char owmURL[256];

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

unsigned int localPort = 2390;      // local port to listen for UDP packets
const char* ntpServerName = "time.google.com";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE] = { 0 };
WiFiUDP udp;

void sendNTPpacket(IPAddress& address) {
  if (WiFi.isConnected()) {
    if (packetBuffer[0] == 0) { // it once for all
      memset(packetBuffer, 0, NTP_PACKET_SIZE);
      packetBuffer[0] = 0b11100011;   // LI, Version, Mode
      packetBuffer[1] = 0;     // Stratum, or type of clock
      packetBuffer[2] = 6;     // Polling Interval
      packetBuffer[3] = 0xEC;  // Peer Clock Precision
      // 8 bytes of zero for Root Delay & Root Dispersion
      packetBuffer[12] = 49;
      packetBuffer[13] = 0x4E;
      packetBuffer[14] = 49;
      packetBuffer[15] = 52;
    }
    udp.beginPacket(address, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
  }
}

time_t getNtpTime() {
  if (WiFi.isConnected()) {
    IPAddress timeServerIP; // time.nist.gov NTP server address
    WiFi.hostByName(ntpServerName, timeServerIP);
    while (udp.parsePacket() > 0) ; // discard any previously received packets
    sendNTPpacket(timeServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
      int size = udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
 //       Serial.println("Receive NTP Response");
        udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL;
      }
    }
  }
//  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
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

/*
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
*/

void getOWMInfo() {
  OWMInfo.error = true;
  if (WiFi.isConnected()) {
    Serial.println(owmURL);
    owm.begin(wclient, owmURL);
    if (owm.GET()) {
      String json = owm.getString();
      Serial.println(json);
//     Serial.println(json.length());
      DynamicJsonDocument doc(2048);
      auto error = deserializeJson(doc, json);
      if (!error) {
        float temp = doc["main"]["temp"];
        OWMInfo.temperature = round(temp);
        JsonObject weather = doc["weather"][0];
        strcpy(OWMInfo.description, weather["main"]);
        static char utf8buf[64];
        strcpy(utf8buf, weather["description"]);
         utf8_to_latin9(OWMInfo.fulldescription, utf8buf, strlen(utf8buf));
        strcpy(OWMInfo.icon, weather["icon"]);
	      OWMInfo.error = false;
      } else {
        strcpy(OWMInfo.description, "json");
      }
    }
    owm.end();
  }
}

void initOTA() {
    ArduinoOTA.setHostname("WemosSHT");
    ArduinoOTA.setPassword("wemos");
    ArduinoOTA.onStart([]() {});
    ArduinoOTA.onEnd([]() {});
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      display.setFont(&pixel_millennium_regular5pt8b);
      display.clearDisplay();
      unsigned int realprogress = (progress / (total / 100));
      static char buffer[8];
      sprintf(buffer, "OTA : %.2d %%", realprogress);
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
//  Wire.setClock(400000);

  display.setRotation(0); // 180 degrees
   // show splash screen
   display.clearDisplay();
   display.drawBitmap(0, 0, wemos_logo_64x48_normal, 64, 48, WHITE);
  display.display();
  display.setTextColor(WHITE, BLACK);

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
//  wifiManager.setDebugOutput(true);
  wifiManager.setMinimumSignalQuality(30);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("wemos"); //  no password

  initSHT();
  initOWM();

Serial.println("Ok");

  if (WiFi.isConnected()) {
    udp.begin(localPort);
    setSyncProvider(getNtpTime);
    setSyncInterval(3600);


    initOTA();
    // start webserver
    webServer.on("/orientation", HTTP_POST, []() {
      String value = webServer.arg("value");
      display.setRotation(value.toInt());
     display.display();
    });
    webServer.on("/info", HTTP_GET, []() { // [{"value":"20.50"}]
      const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1);
      DynamicJsonDocument doc(capacity);

      JsonObject arr = doc.createNestedObject();
      arr["value"] = SHTInfo.temperature;
      char json[256];
      serializeJsonPretty(doc, json);
      webServer.send(200, "text/json", json);
    });
       webServer.begin();
    MDNS.begin("WemosSHT");
   }
  updateSHT();
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
    webServer.handleClient();
    ArduinoOTA.handle();
    MDNS.update();
    unsigned long currentTime = millis();

    if ((currentTime - lastDisplayTime) >= 4 * 1000) { // change display every 4s
      Serial.println("Loop2");

       displayContents();
      lastDisplayTime = currentTime;
    }
    if ((lastUpdateTime == 0) || ((currentTime - lastUpdateTime) >= 30 * 1000)) { // check for new values every 30s
      updateSHT();
       Serial.println("Loop3");
     getOWMInfo();
       Serial.println("Loop4");
     sendMQTTValues();
      Serial.println("Loop5");
      lastUpdateTime = currentTime;
   }
}

// Exponential moving average
class Ema {
public:
	Ema(float alpha) : _alpha(alpha) {}
	float filter(float input) {
    if (_hasInitial) {
		  _output = _alpha * (input - _output) + _output;
	  } else {
      _output = input;
      _hasInitial = true;
	  }
	  return _output;
  }
  void reset() {
    _hasInitial = true;
  }
private:
	bool _hasInitial = false;
	float _output = 0;
	float _alpha = 0; //  Smoothing factor, in range [0,1]. Higher the value - less smoothing (higher the latest reading impact)
};

void updateSHT() {
 static Ema emat(0.1);
  static Ema emah(0.1);

  SHTInfo.error = true;
  int error = sht30.get();
  if (error == 0) {
    float temp = sht30.cTemp;
    float humi = sht30.humidity;
      SHTInfo.temperature = emat.filter(temp);
    SHTInfo.humidity = emah.filter(humi);
    SHTInfo.error = false;
    Serial.println(String("temperature:") + temp + ",filtered:" + SHTInfo.temperature);
  //Serial.println(String(",humidity:") + humi + ",fitered humidity:" + SHTInfo.humidity);
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

void displayOWMTemp() {
  display.setFont(&Lato_Medium12pt8b);
  if (OWMInfo.error == false) {
    displayTemp(OWMInfo.temperature);
  } else {
    displayTextCenter(OWMInfo.description);
  }
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
    displayTextCenter(OWMInfo.description);
  }
}

// Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;

void displayNTPTime() {
  display.setFont(&DS_DIGIB12pt7b);
   time_t utc = now();
   time_t local = CE.toLocal(utc, &tcr);
   static char buffer[8];
   sprintf(buffer, "%.2d:%.2d", hour(local), minute(local));
   displayTextCenter(buffer);
}

const char* days[] = { "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"  };
//const char* monthes[] = { "Janvier", "F�vrier", "Mars", "Avril", "Mai", "Juin", "Juillet", 
//                          "Ao�t", "Septembre", "Octobre", "Novembre", "D�cembre" };
  
void displayNTPDate() {
    display.setFont(&Lato_Medium8pt8b);
    time_t utc = now();
     time_t local = CE.toLocal(utc, &tcr);
     char buffer[32];
     sprintf(buffer, "%s %d", days[weekday(local) - 1], day(local)); // , monthes[month(local) -1]);
     displayTextCenter(buffer);
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
