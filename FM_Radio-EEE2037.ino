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

// Handels Phyiscal Buttons
class Button {
  private:
    int pin;
    bool State;
    bool pressed;
    char password[20];
  public:
    Button(int _pin): pin(_pin), State(1)
    {
       pinMode(pin, INPUT_PULLUP);
    }
    void updateState(void)
    {
      bool newState = digitalRead(pin);
      if(State == HIGH && newState == LOW)
      {
        //State = newState;
        pressed = true;
      }else if(State == LOW && newState == HIGH)
      {
        //State = newState;
      }
      State = newState;
    }  
    bool wasPressed(void)
    {
      if(pressed)
      {
        pressed = false;
        return true;
      }
      return false;
    }
};

struct Metadata {
    int8_t version;
} meta;

// Function Prototypes
void startLCD(LiquidCrystal_I2C & LCD);     // Starts the LCD display
void runLCD(LiquidCrystal_I2C & LCD);       // Runs every loop and handles displaying

void startConfigManager(void);              // Starts the ConfigManager (used for Wifi)
void initCallback(void);
void createCustomRoute(WebServer *server);  // How instructions are sent to arduino

void startAR1010(void);                     // Starts the AR1010 (Not yet implemented)

  // Prototypes for Buttons/Website/App
void changeVolume(int newVol);              // Sets Volume
void stepVolume(char dir);                  // Steps volume Up (u) or Down (d)

void setFreq(int freq);                      // Sets Frequency (10ths of MHz)
void stepChannel(char dir);                 // Steps using AR1010 tune Up (u) or Down (d)

void restMem(bool reboot);                  // Resets wifipass code and anything else, allows for restart
    // Prototypes for Interupts

// All Buttons
Button volUp(14);
Button volDown(12);
Button tuneUp(13);
Button tuneDown(15);

// Various States
int volume(4); //Should be between 0 and 18 at all times
//int frequency;


void setup() {

  Wire.begin();
  startLCD(lcd);
  startAR1010();
  startConfigManager();

  configManager.setAPFilename("/index.html");

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
  volUp.updateState();
  volDown.updateState();
  tuneUp.updateState();
  tuneDown.updateState();

  if(volUp.wasPressed())
  {
    stepVolume('u');
    DebugPrintln(volume);
  }
  if(volDown.wasPressed())
  {
    stepVolume('d'); 
    DebugPrintln(volume);
  }
  if(tuneUp.wasPressed())
  {
    //stepChannel('u');
    DebugPrintln(volume);
  }
  if(tuneDown.wasPressed())
  {
    //stepChannel('d');
    DebugPrintln(volume);
  }

  runLCD(lcd);
  configManager.loop();
}

// Turns on LCD and Displays Hello World
void startLCD(LiquidCrystal_I2C &LCD)
{
  LCD.begin(16, 2);
  LCD.init();
  LCD.clear();
  
  // Turn on the backlight.
  LCD.backlight();
}

void runLCD(LiquidCrystal_I2C &LCD)
{
    if(WiFi.status() != WL_CONNECTED)
    {
    LCD.setCursor(0, 0);
    LCD.print("Set-Up: FM Group 4 Box");
    LCD.setCursor(0, 1);
    LCD.print(WiFi.softAPIP());
    }
    else if(WiFi.status() == WL_CONNECTED)
    {
    LCD.setCursor(0, 0);
    LCD.print("SSID:" + WiFi.SSID());
    LCD.setCursor(0, 1);
    LCD.print(WiFi.localIP());
    }
    else
    {
    LCD.setCursor(0, 0);
    LCD.print("ERROR Wifi");
    LCD.setCursor(0, 1);
    LCD.print(WiFi.status());
    }
  
}

// The Config Manager used to connect the Radio to AP without saving the SSID and Password to firmware
void startConfigManager()
{
  DEBUG_MODE = true; // will enable debugging and log to serial monitor
    Serial.begin(115200);
    DebugPrintln("");

    meta.version = 3;

    // Setup config manager  
    configManager.setAPName("FM Group 4 Box");
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

    runLCD(lcd);
  
    configManager.clearWifiSettings(false);
}

void initCallback() {
    config.enabled = false;
    configManager.save();
}

bool toggle = LOW;

// How the user sends request to the Arduino 
void createCustomRoute(WebServer *server) {
    server->on("/volUp", HTTPMethod::HTTP_GET, [server](){
        stepVolume('u');
        server->send(200, "text/plain", "Vol Up");
    });
    server->on("/volDown", HTTPMethod::HTTP_GET, [server](){
        stepVolume('d');
        server->send(200, "text/plain", "Vol Down");
    });
    server->on("/tuneUp", HTTPMethod::HTTP_GET, [server](){
        stepChannel('u');
        server->send(200, "text/plain", "Hello, World!");
    });
    server->on("/tuneDown", HTTPMethod::HTTP_GET, [server](){
        stepChannel('d');
    });
    server->on("/ledToggle", HTTPMethod::HTTP_GET, [server](){
        digitalWrite(BUILTIN_LED, toggle);
        toggle = !toggle;
        server->send(200, "text/plain", "Toggle");
    });
}

// Starts the AR1010 I2C Communications
void startAR1010()
{
  //radio.initialise();
  delay(1000);

  radio.setFrequency(1046);
  radio.setVolume(9);
}


// Handels all User Input
void changeVolume(int newVol)              // Sets Volume
{
  if(newVol < 18 && newVol > 0)
  {
    volume = newVol;
    radio.setVolume(volume);
  }
}

void stepVolume(char dir)                  // Steps volume Up (u) or Down (d)
{
  if(dir == 'd' && volume > 0)
  {
    volume--;
  }
  else if(dir == 'u' && volume < 18)
  {
    volume++;
  }
  radio.setVolume(volume);
}

void setFreq(int freq)                      // Sets Frequency (10ths of MHz)
{
  radio.setFrequency(freq);  
}

void stepChannel(char dir)                  // Steps using AR1010 tune Up (u) or Down (d)
{
  if(dir == 'd' || dir == 'u')
  {
    radio.seek(dir);
  }
  else
  {
    DebugPrintln("Invalid Dir"); 
  }
}

void restMem(bool reboot)                   // Resets Memory and possibly reboots
{
  
}

// Interupts
