#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

String host = "blr1.blynk.cloud";
int httpPort = 80;  
String doorURL = "/external/api/update?token=mcw5pFEwP3R8zENEMHVozkcNDbXW5JS8&v1=1";
String doorStatusURL = "/external/api/isHardwareConnected?token=mcw5pFEwP3R8zENEMHVozkcNDbXW5JS8";

WiFiClient client;
HTTPClient http;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

bool buttonState[2];                                      //the current reading from the input pin: door, function
bool lastButtonState[2] = {0, 0};                         //the previous reading from the input pin: door, function
unsigned long lastDebounceTime[3] = {0, 0};               //the last time the output pin was toggled: door, function
unsigned long timing[3] = {0, 0, 0};                      //batteryBlinking, batteryPercentageUpdate, chargingAnimation
const int n = 3;                                          //Number of wifi credential to save
int a = 0;                                                //Used to know the Active wifi network    
bool displayState = 1;                                    //Used to turn on or off display
bool buttonPress = 0;                                     //Flag for button press while connection
bool blink = 0;                                           //Help in blinking icons in display
int width = 0;                                            //Used in changing the width of battery symbol during charging
int brightness = 0;                                       //Used to control brightness of Built_In_Led
int fadeAmount = 3;                                       //Used to fade the Built_In_Led for animation

#define door D0  
#define function D5                                      
#define battery A0                                        //For battery capacity checking

String credential[n][2] = {{"somnath", "Ganpati@1942"}, {"Sourabh", "Sampa@123"}, {"BeamCruzer", "TumMeraDostHo"}};
const int buttonPin[2] = {door, function};
float calibration = 0.35;                                 //Calibration required due to error in resistance values in battery voltage divider
int batteryLevel;                                         //Stores the current battery level
int batteryWidth;                                         //Stores the battery symbol width


void setup()
{
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);

  pinMode(buttonPin[0], INPUT_PULLDOWN_16);               //No Pulldown resistor required
  pinMode(buttonPin[1], INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  connection();                                           //Connects the wifi
}


void loop() 
{
  while (WiFi.status() == WL_CONNECTED)
  {
    display.clearDisplay();
    wifiSymbol();
    batterySymbol();
    realTime();
    fader();
    if (displayState == 1) display.display();

    for (int i=0; i<2; i++)
    {
      checkButton(i);
    }
  }  
  connection();                                           //Reconnects the wifi in case of disconnection
}


void checkButton(int i)
{
  int reading = digitalRead(buttonPin[i]);
  if (reading != lastButtonState[i]) 
  {
    lastDebounceTime[i] = millis();
  }

  if ((millis() - lastDebounceTime[i]) > 50 && reading != buttonState[i]) 
  {
    buttonState[i] = reading;
    if (buttonState[i] == HIGH) 
    {
      if (i == 0 && WiFi.status() == WL_CONNECTED)
      {
        // Comment the below two lines during troubleshooting to stop the working of APIs
        http.begin(client,host,httpPort,doorURL);              
        http.GET();
        if (displayState == 1) animation();
      }

      else if (i == 1 && WiFi.status() == WL_CONNECTED)
      {
        displayState = !displayState;
        display.clearDisplay();
        display.display();
      }

      else if (i == 1 && WiFi.status() != WL_CONNECTED)
      {
        buttonPress = 1;
      }
    }
  }
  lastButtonState[i] = reading;
}


void connection()
{
  display.clearDisplay();
  a = 0;                                          //Sets the priority of the wifi network to connect first
  buttonPress = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(credential[a][0], credential[a][1]);
    while(WiFi.status() == 7)                                                     //7 means Listening mode
    {
      checkButton(1);
      if (buttonPress == 1)
      {
        (a < n-1) ? (a++) : (a=0);
        buttonPress = 0;
        break;
      }
      delay(200);
      display.fillRect(0, 0, 128, 40, BLACK);
      display.display();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.println("Connecting with\n");
      display.setTextSize(2);
      display.println(credential[a][0]);
      display.display();
    }
  }

  timeClient.begin();
  display.clearDisplay();
  display.setCursor(0,10);
  display.setTextSize(1);
  display.println("Connected with\n");
  display.setTextSize(2);
  display.println(credential[a][0]);
  display.setTextSize(1);
  display.println(WiFi.localIP());
  display.display();
  delay(1000);
  DeviceConnectedChecker();
}


void DeviceConnectedChecker()
{
  http.begin(client,host,httpPort,doorStatusURL); 
  http.GET();
  String payload = http.getString();
  if (payload == "false")
  {
    delay(1000);
    display.clearDisplay();
    display.setCursor(0, 25);
    display.println("Door Control is\nnot connected!");
    display.display();
    delay(2000);
  }
}


void wifiSymbol()
{
  // Draw the Wi-Fi symbol
  display.drawLine(1, 2, 3, 0, WHITE);
  display.drawLine(3, 0, 5, 2, WHITE);
  display.drawLine(5, 2, 7, 0, WHITE);
  display.drawLine(7, 0, 9, 2, WHITE);
  display.drawLine(1, 6, 3, 8, WHITE);
  display.drawLine(3, 8, 5, 6, WHITE);
  display.drawLine(5, 6, 7, 8, WHITE);
  display.drawLine(7, 8, 9, 6, WHITE);
  display.setTextSize(1);
  display.setCursor(13, 1);
  display.print(credential[a][0]);
}


void batterySymbol()
{ 
  // Draw the battery symbol outline
  display.drawRect(105, 0, 20, 8, WHITE);  // Battery outline
  display.drawLine(125, 2, 125, 6, WHITE); // Positive terminal
  int currentBatteryLevel = batteryCapacity();

  // Stores the battery level every 5 sec to avoid fluctuations
  if ((millis()-timing[1]) > 5000 && currentBatteryLevel <= 100)
  {
    // Calculate the width of the battery level based on the current battery level and maximum capacity
    batteryLevel = batteryCapacity();
    batteryWidth = (int)((batteryLevel / 100.0) * 16);
    timing[1] = millis();
  }


  if (currentBatteryLevel <= 100)                             //Discharging state
  { 
    // Draw the battery level
    display.fillRect(107, 2, batteryWidth, 4, WHITE);
  }

  else                                                        //Charging state
  {
    // Charging animation in 4 steps if batteryLevel is greater than 100
    display.fillRect(107, 2, width, 4, WHITE);
    if(millis() - timing[2] >= 700)
    {
      width = (width < 16) ? width + 4 : 0;
      timing[2] = millis();
    }
  }

  // Draw the battery percentage on top left just below the symbol
  int x = (batteryLevel >= 100) ? 104 : 110;

  if (currentBatteryLevel <= 30 && (millis() - timing[0]) > 700)
  {
    blink = !blink;
    display.setTextColor(blink ? BLACK : WHITE);
    display.setCursor(x, 10);
    display.print(batteryLevel);
    display.print("%");
    display.setTextColor(WHITE);
    timing[0] = millis();
  }

  else if (30 < currentBatteryLevel && currentBatteryLevel <= 100)
  {
    display.setCursor(x, 10);
    display.print(batteryLevel);
    display.print("%");
  }
}


int batteryCapacity()
{
  int sensorValue = analogRead(battery);

  //multiply by two as voltage divider network is 100K & 100K Resistor
  float voltage = (((sensorValue * 3.3) / 1024) * 2 + calibration);

  //Uncomment to match voltage with actual battery voltage and set the calibration as per requirement
  // Serial.println(voltage);

  //2.8V as Battery Cut off Voltage & 4.2V as Maximum Voltage
  int percentage = mapFloat(voltage, 3.2, 4, 0, 100);
  return (percentage);
}


float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void animation()
{
  // Define the vertical position of each letter
  int B_pos = 27;
  int A_pos = 35;
  int M_pos = 43;
  
  // Draw the letters "BAM" vertically stacked
  display.setCursor(113, B_pos);
  display.println("B");
  display.setCursor(113, A_pos);
  display.println("A");
  display.setCursor(113, M_pos);
  display.println("M");
  display.display();
  delay(500);

  //Fill with white rectangle
  display.fillRect(111, B_pos-2, 9, 27, WHITE);
  display.display();


  //Draw the same letters back in black color
  display.setTextColor(BLACK);
  display.setCursor(113, B_pos);
  display.println("B");
  display.setCursor(113, A_pos);
  display.println("A");
  display.setCursor(113, M_pos);
  display.println("M");
  display.display();
  display.setTextColor(WHITE);
  delay(200);

  //Clear display
  display.fillRect(111, B_pos-2, 9, 27, BLACK);
  display.display();
}


void realTime()
{
  timeClient.update();
  display.setCursor(0, 22);
  display.setTextSize(2);
  display.print(timeClient.getFormattedTime());

  //Week Days
  String wd[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  //Months
  String m[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  String weekDay = wd[timeClient.getDay()];
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  String monthDay = String(ptm->tm_mday);
  String month = m[(ptm->tm_mon+1)-1];
  int year = ptm->tm_year+1900;

  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print(monthDay + " " + month + " " + year);
  display.setCursor(0, 54);
  display.print(weekDay);
}


void fader()
{
  analogWrite(LED_BUILTIN, brightness);
  brightness += fadeAmount;

  if (brightness <= 0 || brightness >= 255)
    fadeAmount = -fadeAmount;
  if (displayState == 0) delay(40);
}
