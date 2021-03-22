/*
 * FM Radio Project Box 4
 * Created by Alexandre Symeonidis-Herzig 2021
*/
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ConfigManager.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ar1010lib.h>
#include <WiFiClient.h>

#define MAGIC_LENGTH 2

// Global Instances
ConfigManager configManager;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Requires the address
WiFiServer server(80);
AR1010 radio = AR1010();
 
// Settings for ConfigManager
struct Config {
    char name[20];
    bool enabled;
    int8_t hour;
    char password[20];
} config;

struct Metadata {
    int8_t version;
} meta;

// Function Prototypes
void startLCD(void); // Starts the LCD display
void startConfigManager(void); // Starts the ConfigManager (used for Wifi)
void startAR1010(void); // Starts the AR1010 (Not yet implemented)
void initCallback(void);
void createCustomRoute(WebServer *server); // How instructions are sent to arduino

void setup() {

  startLCD();
  startAR1010();
  startConfigManager();
  
  lcd.setCursor(0, 0);
  lcd.print("SSID:" + WiFi.SSID());
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  configManager.setAPFilename("/control.html");


  /*
   * For Debugging
   */
  Serial.begin(115200);
  delay(10);
  // prepare GPIO
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, 0);

  
}

void loop() 
{

 configManager.loop();
}

// Turns on LCD and Displays Hello World
void startLCD()
{
  lcd.begin(16, 2);
  lcd.init();
  lcd.clear();
  
  // Turn on the backlight.
  lcd.backlight();
}

// The Config Manager used to connect the Radio to AP without saving the SSID and Password to firmware
void startConfigManager()
{
  DEBUG_MODE = true; // will enable debugging and log to serial monitor
    Serial.begin(115200);
    DebugPrintln("");

    meta.version = 3;

    // Setup config manager
    configManager.setAPName("FM Radio Box 4");
    configManager.setAPFilename("/index.html");
    configManager.addParameter("name", config.name, 20);
    configManager.addParameter("enabled", &config.enabled);
    configManager.addParameter("hour", &config.hour);
    configManager.addParameter("password", config.password, 20, set);
    configManager.addParameter("version", &meta.version, get);

    // Create custom routes to serve via callback hooks
    configManager.setAPCallback(createCustomRoute);
    configManager.setAPICallback(createCustomRoute);

    // Create a initialization hook
    // Will run until a memory write is performed.
    configManager.setInitCallback(initCallback);

    configManager.begin(config);
}

void initCallback() {
    config.enabled = false;
    configManager.save();
}

// How the user sends request to the Arduino 
void createCustomRoute(WebServer *server) {
    server->on("/custom", HTTPMethod::HTTP_GET, [server](){
      digitalWrite(BUILTIN_LED, 1);
        server->send(200, "text/plain", "Hello, World!");
    });
    server->on("/test", HTTPMethod::HTTP_GET, [server](){
        digitalWrite(BUILTIN_LED, 0);
        server->send(200, "text/plain", "Radio?");
    });
}

// Starts the AR1010 I2C Communications
void startAR1010()
{
  1 == 1; 
}
