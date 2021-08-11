#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include "SPIFFS.h"

#include "marquee.h"
#include "rainbow.h"
#include "twinkle.h"
#include "comet.h"
#include "static.h"
#include "helpers.h"

#define MULTICORE
#define SEM_WAIT_TICKS 20

#define OLED_CLOCK 15
#define OLED_DATA 4
#define OLED_RESET 16

// LED Strip
#define NUM_LEDS 144
#define LED_PIN 18

CRGB g_LEDs[NUM_LEDS] = {0}; // Frame buffer for FastLED

enum StripEffect {
  RAINBOW, MARQUEE, TWINKLE, COMET, FLUIDMARQUEE, FLUIDCOMET, STATIC
};

struct StripConfig {
  bool on; // true if the strip is turned on
  StripEffect currentEffect;
  uint8_t brightness;
  uint8_t speed;
  CRGB color;
};
static struct StripConfig stripConf = { true, StripEffect::FLUIDMARQUEE, 255, 1, CRGB::White };
SemaphoreHandle_t  stripConf_sem; 

// Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_OLED(U8G2_R2, OLED_RESET, OLED_CLOCK, OLED_DATA);
int g_lineHeight = 0;

// WiFi
char SSID[50];           
char WIFI_PASSWORD[50]; 
const int MAX_WIFI_CONNECTION_RETRIES = 50;

// MQTT
char MQTT_USERNAME[50];
char MQTT_PASSWORD[50];
char MQTT_BROKER_ADDRESS[100];
int MQTT_BROKER_PORT;
const char* ROOT_CA = \
  "-----BEGIN CERTIFICATE-----\n"
  "MIIEZTCCA02gAwIBAgIQQAF1BIMUpMghjISpDBbN3zANBgkqhkiG9w0BAQsFADA/\n" \
  "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
  "DkRTVCBSb290IENBIFgzMB4XDTIwMTAwNzE5MjE0MFoXDTIxMDkyOTE5MjE0MFow\n" \
  "MjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxCzAJBgNVBAMT\n" \
  "AlIzMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuwIVKMz2oJTTDxLs\n" \
  "jVWSw/iC8ZmmekKIp10mqrUrucVMsa+Oa/l1yKPXD0eUFFU1V4yeqKI5GfWCPEKp\n" \
  "Tm71O8Mu243AsFzzWTjn7c9p8FoLG77AlCQlh/o3cbMT5xys4Zvv2+Q7RVJFlqnB\n" \
  "U840yFLuta7tj95gcOKlVKu2bQ6XpUA0ayvTvGbrZjR8+muLj1cpmfgwF126cm/7\n" \
  "gcWt0oZYPRfH5wm78Sv3htzB2nFd1EbjzK0lwYi8YGd1ZrPxGPeiXOZT/zqItkel\n" \
  "/xMY6pgJdz+dU/nPAeX1pnAXFK9jpP+Zs5Od3FOnBv5IhR2haa4ldbsTzFID9e1R\n" \
  "oYvbFQIDAQABo4IBaDCCAWQwEgYDVR0TAQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8E\n" \
  "BAMCAYYwSwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5p\n" \
  "ZGVudHJ1c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTE\n" \
  "p7Gkeyxx+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEE\n" \
  "AYLfEwEBATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2Vu\n" \
  "Y3J5cHQub3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0\n" \
  "LmNvbS9EU1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYf\n" \
  "r52LFMLGMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjANBgkqhkiG9w0B\n" \
  "AQsFAAOCAQEA2UzgyfWEiDcx27sT4rP8i2tiEmxYt0l+PAK3qB8oYevO4C5z70kH\n" \
  "ejWEHx2taPDY/laBL21/WKZuNTYQHHPD5b1tXgHXbnL7KqC401dk5VvCadTQsvd8\n" \
  "S8MXjohyc9z9/G2948kLjmE6Flh9dDYrVYA9x2O+hEPGOaEOa1eePynBgPayvUfL\n" \
  "qjBstzLhWVQLGAkXXmNs+5ZnPBxzDJOLxhF2JIbeQAcH5H0tZrUlo5ZYyOqA7s9p\n" \
  "O5b85o3AM/OJ+CktFBQtfvBhcJVd9wvlwPsk+uyOy2HI7mNxKKgsBTt375teA2Tw\n" \
  "UdHkhVNcsAKX1H7GNNLOEADksd86wuoXvg==\n" \
  "-----END CERTIFICATE-----\n";
char MQTT_CLIENT_ID[50];
char OWNER[50];
char ROOM[50];
char NAME [50];
char mqtt_strip_topic[200] = "\0";
char mqtt_controller_topic[200] = "\0";
const int MAX_PAYLOAD_SIZE = 500;
const int MAX_MQTT_CONNECTION_RETRIES = 5;
WiFiClientSecure espClientSecure;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// General
int MAIN_LOOP_PERIOD = 2000; // in milliseconds
int LED_LOOP_PERIOD = 17; // in milliseconds
const int MAX_SETTINGS_FILE_SIZE = 1000;

#ifdef MULTICORE
TaskHandle_t MQTTManagement;
#endif

void haltExecution() {
  while(true) {
    delay(1);
  }
}

void displayPrint(const char *lines[], int length) {
  g_OLED.clear();
  for (int i = 0; i < length; i++) {
    g_OLED.setCursor(0, g_lineHeight * (i + 1));
    g_OLED.print(lines[i]);
  }
  g_OLED.sendBuffer();
}

int clamp(int value, int min, int max) {
  if (value < min) {
    return min;
  }

  if (value > max) {
    return max;
  }

  return value;
}

void DrawStripEffect(CRGB* g_LEDs, int numLEDs, int deltaTime) {
  if (xSemaphoreTake(stripConf_sem, SEM_WAIT_TICKS) == pdTRUE) {
    if (stripConf.on) {
      switch (stripConf.currentEffect) {
        case StripEffect::RAINBOW: {
          DrawRainbow(g_LEDs, numLEDs, deltaTime, stripConf.speed, Direction::COUNTERCLOCKWISE);
          break;
        } 
        case StripEffect::MARQUEE: {
          DrawMarquee(g_LEDs, numLEDs, deltaTime, stripConf.speed);
          break;
        }
        case StripEffect::TWINKLE: {
          DrawTwinkle(g_LEDs, numLEDs, deltaTime, stripConf.speed);
          break;
        }
        case StripEffect::COMET: {
          DrawComet(g_LEDs, numLEDs, deltaTime, stripConf.speed, 7);
          break;
        }
        case StripEffect::FLUIDMARQUEE: {
          DrawFluidMarquee(g_LEDs, numLEDs, deltaTime, stripConf.speed, stripConf.color);
          break;
        }
        case StripEffect::FLUIDCOMET: {
          DrawFluidComet(g_LEDs, numLEDs, deltaTime, stripConf.speed, 7, stripConf.color);
          break;
        }
        case StripEffect::STATIC: {
          DrawStatic(g_LEDs, numLEDs, deltaTime, stripConf.speed, stripConf.color);
          break;
        }
      }
    }
    xSemaphoreGive(stripConf_sem);
  } else {
    Serial.println("LED LOOP: COULDN'T TAKE SEMAPHORE.");
  }
}

void UpdateStripStatus() {
  FastLED.clear();
  FastLED.setBrightness(stripConf.brightness);
}


void parseNewStripConf(DynamicJsonDocument doc) {
  if (xSemaphoreTake(stripConf_sem, SEM_WAIT_TICKS) == pdTRUE) {
    const JsonVariant newConf = doc["newConf"];
    if (!newConf.as<boolean>()) {
      Serial.println("This configuration is not new and will not be parsed.");
      xSemaphoreGive(stripConf_sem);
      return;
    }

    const char *lines[6] = { "\0" };

    const char* status = doc["status"];
    if (status){
      if (strcmp(status, "on") == 0) {
        stripConf.on = true;
      } else if (strcmp(status, "off") == 0) {
        stripConf.on = false;
      }
    }

    const char* e = doc["effect"];
    if (e) {
      if (strcmp(e, "rainbow") == 0) {
        stripConf.currentEffect = StripEffect::RAINBOW;
      } else if (strcmp(e, "marquee") == 0) {
        stripConf.currentEffect = StripEffect::MARQUEE;
      } else if (strcmp(e, "fluidmarquee") == 0) {
        stripConf.currentEffect = StripEffect::FLUIDMARQUEE;
      } else if (strcmp(e, "twinkle") == 0) {
        stripConf.currentEffect = StripEffect::TWINKLE;
      } else if (strcmp(e, "comet") == 0) {
        stripConf.currentEffect = StripEffect::COMET;
      } else if (strcmp(e, "fluidcomet") == 0) {
        stripConf.currentEffect = StripEffect::FLUIDCOMET;
      } else if (strcmp(e, "static") == 0) {
        stripConf.currentEffect = StripEffect::STATIC;
      }
    }

    lines[0] = "Strip conf received.";
    lines[1] = status;
    lines[2] = e;
    displayPrint(lines, 3);

    JsonVariant b = doc["brightness"];
    if (!b.isNull()) {
      stripConf.brightness = (uint8_t) clamp(b.as<int>(), 0, 255);
      Serial.printf("\nNew brightness: %d\n", stripConf.brightness);
    }

    JsonVariant s = doc["speed"];
    if (!s.isNull()) {
      stripConf.speed = (uint8_t) clamp(s.as<int>(), 1, 10);
      Serial.printf("\nNew speed: %d\n", stripConf.speed);
    } 

      JsonVariant c = doc["color"];
      if (!c.isNull()) {
        stripConf.color.setRGB((uint8_t) clamp(c["r"].as<int>(), 0, 255),
                                (uint8_t) clamp(c["g"].as<int>(), 0, 255),
                                (uint8_t) clamp(c["b"].as<int>(), 0, 255));
        Serial.printf("\nNew color: (%d, %d, %d)\n", stripConf.color.r, stripConf.color.g, stripConf.color.b);
      }

    UpdateStripStatus();
    xSemaphoreGive(stripConf_sem);
  } else {
    Serial.println("MQTT CALLBACK: COULDN'T TAKE SEMAPHORE.");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]\n");

  DynamicJsonDocument doc(MAX_PAYLOAD_SIZE);
  char decodedPayload[MAX_PAYLOAD_SIZE];

  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    decodedPayload[i] = (char)payload[i];
  }
  decodedPayload[length] = '\0';

  DeserializationError error = deserializeJson(doc, decodedPayload);
  if (error) {
    Serial.print(F("MQTT Payload deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  if (strcmp(topic, mqtt_strip_topic) == 0) {
    parseNewStripConf(doc);
    Serial.println();
  } else if (strcmp(topic, mqtt_controller_topic) == 0) {
    // parse controller configuration
  } else {
    Serial.print("Received message on unrecognised topic: ");
    Serial.println(topic);
  }

  
}


void reconnect() {
  const char *lines[6] = { "\0" };
  int retryCounter = 0;
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    lines[0] = "Connecting to";
    lines[1] = "MQTT broker...";
    displayPrint(lines, 2);

    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      lines[0] = "MQTT connection";
      lines[1] = "was successful.";
      lines[2] = "Client ID:";
      lines[3] = MQTT_CLIENT_ID;
      displayPrint(lines, 4);

      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish(mqtt_strip_topic, "Connected!");
      // ... and resubscribe
      boolean subscribeSuccess = mqttClient.subscribe(mqtt_controller_topic);
      if (!subscribeSuccess) {
        Serial.println("Error while subscribing to controller topic!");
      }
      subscribeSuccess = mqttClient.subscribe(mqtt_strip_topic);
      if (!subscribeSuccess) {
        Serial.println("Error while subscribing to strip topic!");
      }
    } else {
      lines[0] = "MQTT connection";
      lines[1] = "failed.";
      char reason[6] = "";
      sprintf(reason, "rc=%d", mqttClient.state());
      lines[2] = reason;
      lines[3] = "Retrying...";
      displayPrint(lines, 4);

      retryCounter++;
      if (retryCounter >= MAX_MQTT_CONNECTION_RETRIES) {
        ESP.restart();
      }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void scanNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") Network: ");
    Serial.println(WiFi.SSID(thisNet));
  }
}

void wifiSetup() {
  const char* lines[4] = { "\0" };

  lines[0] = "Connecting to";
  lines[1] = "WiFi network:";
  lines[2] = SSID;
  displayPrint(lines, 3);

  WiFi.disconnect();
  delay(1000);

  scanNetworks();

  WiFi.begin(SSID, WIFI_PASSWORD);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_IDLE_STATUS && WiFi.localIP().toString() == "0.0.0.0") {
    delay(500);
    Serial.printf("Status: %d\n", WiFi.status());
    Serial.println("Trying to connect to WiFi network...");
    g_OLED.setCursor(i, g_lineHeight * 4);
    g_OLED.printf(".");
    i += 1;

    if (i >= MAX_WIFI_CONNECTION_RETRIES) {
      ESP.restart();
    }
  }

  Serial.print("Connected. IP Address: ");
  Serial.println(WiFi.localIP().toString());

  g_OLED.clear();
  g_OLED.setCursor(0, g_lineHeight);
  g_OLED.print("Connected.");
  g_OLED.setCursor(0, g_lineHeight * 2);
  g_OLED.print("IP Address:");
  g_OLED.setCursor(0, g_lineHeight * 3);
  g_OLED.print(WiFi.localIP().toString().c_str());
  g_OLED.sendBuffer();
}

bool parseSettings() {
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS.");
    return false;
  }
  
  File file = SPIFFS.open("/settings.json");
  if(!file){
    Serial.println("Failed to open settings file for reading.");
    return false;
  }
  
  Serial.println("Settings File Content:");
  char settingsContent[MAX_SETTINGS_FILE_SIZE];
  int i = 0;
  while(file.available()){
    int c = file.read();
    Serial.write(c);
    settingsContent[i] = (char)c;
    i++;
  }
  settingsContent[i] = '\0';
  DynamicJsonDocument doc(i);
  DeserializationError error = deserializeJson(doc, settingsContent);

  if (error) {
    Serial.print(F("Settings File deserializeJson() failed: "));
    Serial.println(error.f_str());
    file.close();
    return false;
  }
  Serial.print('\n');

  // TODO: handle missing keys
  strcpy(SSID, doc["wifi"]["ssid"]);
  strcpy(WIFI_PASSWORD, doc["wifi"]["password"]);
  strcpy(MQTT_USERNAME, doc["mqtt"]["username"]);
  strcpy(MQTT_PASSWORD, doc["mqtt"]["password"]);
  strcpy(MQTT_BROKER_ADDRESS, doc["mqtt"]["brokerAddress"]);
  MQTT_BROKER_PORT = doc["mqtt"]["brokerPort"].as<int>();
  strcpy(MQTT_CLIENT_ID, doc["mqtt"]["clientID"]);
  strcpy(OWNER, doc["mqtt"]["owner"]);
  strcpy(ROOM, doc["mqtt"]["room"]);
  strcpy(NAME, doc["mqtt"]["name"]);

  MAIN_LOOP_PERIOD = doc["general"]["loopPeriodMs"].as<int>();
  LED_LOOP_PERIOD = doc["led"]["loopPeriodMs"].as<int>();

  Serial.print("SSID: ");
  Serial.println(SSID);
  Serial.print("Password: ");
  Serial.println(WIFI_PASSWORD);

  file.close();
  Serial.println("Settings parsed.");
  return true;
}

void initDisplay() {
  g_OLED.begin();
  g_OLED.clear();
  g_OLED.setFont(u8g2_font_profont15_tf);

  g_lineHeight = g_OLED.getAscent() - g_OLED.getDescent();

  g_OLED.setCursor(0, g_lineHeight);
  g_OLED.print("Welcome!");
  g_OLED.sendBuffer();
  delay(1000);
}

#ifdef MULTICORE
void MQTTSetup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Display initialization
  initDisplay();

  if (parseSettings()) {
    // WiFi initialization
    wifiSetup();
    delay(1000);

    espClientSecure.setCACert(ROOT_CA);
    espClientSecure.setInsecure(); // this shouldn't be needed
    mqttClient.setServer(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT);
    mqttClient.setCallback(mqttCallback);
    sprintf(mqtt_strip_topic, "%s/%s/%s/strip", OWNER, ROOM, NAME);
    sprintf(mqtt_controller_topic, "%s/%s/%s/controller", OWNER, ROOM, NAME);
    Serial.printf("MQTT Strip Topic: %s\n", mqtt_strip_topic);
    Serial.printf("MQTT Controller Topic: %s\n", mqtt_controller_topic);

    Serial.printf("Started!");
    delay(1000);
  } else {
    const char* lines[3] = { "\0" };
    lines[0] = "Error while";
    lines[1] = "parsing";
    lines[2] = "settings.json!";
    displayPrint(lines, 3);
    haltExecution();
  }
}

const char *stripEffectToString(StripEffect e) {
  if (e == StripEffect::RAINBOW) {
    return "rainbow";
  } else if (e == StripEffect::MARQUEE) {
    return "marquee";
  } else if (e == StripEffect::COMET) {
    return "comet";
  } else if (e == StripEffect::FLUIDCOMET) {
    return "fluidcomet";
  } else if (e == StripEffect::FLUIDMARQUEE) {
    return "fluidmarquee";
  } else if (e == StripEffect::STATIC) {
    return "static";
  } else if (e == StripEffect::TWINKLE) {
    return "twinkle";
  } else {
    Serial.println("WARNING: unknown effect.");
    return "unknown";
  }
}

void PublishStripStatus() {
  char message[2048];
  DynamicJsonDocument currentConf(2048);
  currentConf["status"] = stripConf.on ? "on" : "off";
  char brightness[3] = { '\0' };
  sprintf(brightness, "%d", stripConf.brightness);
  currentConf["brightness"] = brightness;
  char speed[2] = { '\0' };
  sprintf(speed, "%d", stripConf.speed);
  currentConf["speed"] = speed;
  currentConf["effect"] = stripEffectToString(stripConf.currentEffect);

  serializeJsonPretty(currentConf, Serial);
  Serial.print("\n");

  serializeJson(currentConf, message);

  Serial.print("Publishing strip status: ");
  Serial.println(message);
  int length = strlen(message);
  boolean success = mqttClient.publish(mqtt_strip_topic, message, length);

  if (!success) {
    Serial.println("Error while publishing strip status.");
  } else {
    Serial.println("Publish success!");
  }
}

void MQTTLoop(void* pvParameters) {

  MQTTSetup();
  static int lastMainLoopTime = 0;

  for (;;) {
    // MQTT
    if (!mqttClient.connected()) {
      reconnect();
    }

    int now = millis();
    int mainLoopDeltaTime = now - lastMainLoopTime;
    static bool ledStatus = false;

    // main loop
    if (mainLoopDeltaTime > MAIN_LOOP_PERIOD) {
      // put your main code here, to run repeatedly:
      ledStatus = !ledStatus;
      digitalWrite(LED_BUILTIN, ledStatus);
      Serial.println("Hearbeat");

      PublishStripStatus();
    }

    boolean success = mqttClient.loop();
    delay(100);
    if (!success) {
      Serial.println("Error while executing MQTT client loop.");
    }

    if (mainLoopDeltaTime > MAIN_LOOP_PERIOD) {
      lastMainLoopTime = now;
    }

    // vTaskDelay(MAIN_LOOP_PERIOD / (2 * portTICK_PERIOD_MS));
  }
}
#endif

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);

  // FastLED
  Serial.println("Start LED configuration.");
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(g_LEDs, NUM_LEDS).setCorrection(TypicalLEDStrip);
  UpdateStripStatus();

  stripConf_sem = xSemaphoreCreateBinary();
  xSemaphoreGive(stripConf_sem);
  xTaskCreatePinnedToCore(
                    MQTTLoop,   /* Task function. */
                    "MQTT Connection Management",     /* name of task. */
                    50000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &MQTTManagement,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500);
}

void loop() {
  static int lastLEDLoopTime = 0;

  int now = millis();
  int ledLoopDeltaTime = now - lastLEDLoopTime;

  // LEDs loop
  if (ledLoopDeltaTime > LED_LOOP_PERIOD) {
    DrawStripEffect(g_LEDs, NUM_LEDS, ledLoopDeltaTime);
    FastLED.show();
    lastLEDLoopTime = now;
  }
}