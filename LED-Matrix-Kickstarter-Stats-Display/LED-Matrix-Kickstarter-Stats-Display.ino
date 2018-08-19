/*******************************************************************
    Kickstarter Project statistics using an ESP8266 and LED Matrix

    Fetches live kickstarter project data and displays on
    a LED Matrix

    By Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitch: https://www.twitch.tv/brianlough
    Twitter: https://twitter.com/witnessmenow

 ********************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------

#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>


// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------


#include <PxMatrix.h>
// The library for controlling the LED Matrix
// Search for "PxMatrix" in the Arduino Library manager
// https://github.com/2dom/PxMatrix
//
// NOTE: Has a dependancy on "Adafruit GFX" library (install this too)

#include <KickstarterStats.h>
// For fetching the stats from Kickstarter
// Search for "kickstarter" in the Arduino Library manager
// https://github.com/witnessmenow/arduino-kickstarter-stats

#include <ArduinoJson.h>
// This Sketch doesn't technically need this, but the KS library does so it must be installed.
// Search for "ArduinoJson" in the library manger
// NOTE: You must install version 5.X as 6.X has breaking changes
// https://github.com/bblanchon/ArduinoJson

#include <TimeLib.h>
// For handling time
// Needs to be installed manually
// https://github.com/PaulStoffregen/Time

#include <NTPClient.h>
// For getting the time off the NTP server
// Search for "NTPClient" and install the one by Fabrice Weinberg
// https://github.com/arduino-libraries/NTPClient


//------- Replace the following! ------
char ssid[] = "SSID";       // your network SSID (name)
char password[] = "password";  // your network key

#define KS_USER "idlehandsdev" // User name on kickstarter: https://www.kickstarter.com/projects/idlehandsdev 

#define KS_PROJECT "hcc-mod-ready-to-solder-arduino-module" // Project name on kickstarter: 
// https://www.kickstarter.com/projects/idlehandsdev/hcc-mod-ready-to-solder-arduino-module

// This does not come back from the API, but it is a constant per project
const long target = 10000;

const tmElements_t finishTime = {0, 0, 33, 13, 23,  8, CalendarYrToTm(2018)};

const String projectName = "HCC MOD";

//------------------------------

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

Ticker display_ticker;

WiFiClientSecure client;
KickstarterStats kss(client);

unsigned long delayBetweenRequests = 60000; // Time between requests (1 minute)
unsigned long requestDueTime;   //time when request due

unsigned long delayBetweenNTPUpdates = 10000; // Time between requests (10 seconds)
unsigned long ntpUpdateDueTime;   //time when request due

// Pins for LED MATRIX
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
#define P_D 12
#define P_E 0

// PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
// PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

uint16 myCOLORS[8] = {myRED, myGREEN, myBLUE, myWHITE, myYELLOW, myCYAN, myMAGENTA, myBLACK};

long pledged = 0;
long backerCount = 0;

// ISR for display refresh
void display_updater()
{

  display.display(70);

}
void setup() {

  // Define your display layout here, e.g. 1/8 step
  display.begin(16);
  display.clearDisplay();
  display.setTextColor(myCYAN);
  display.setCursor(2, 0);
  display.print("Connecting");
  display.setTextColor(myMAGENTA);
  display.setCursor(2, 8);
  display.print("to the");
  display.setTextColor(myGREEN);
  display.setCursor(2, 16);
  display.print("WiFi!");

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);


  Serial.begin(9600);

  Serial.print("Pixel draw latency in us: ");
  unsigned long start_timer = micros();
  display.drawPixel(1, 1, 0);
  unsigned long delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  Serial.print("Display update latency in us: ");
  start_timer = micros();
  display.display(0);
  delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  display_ticker.attach(0.002, display_updater);
  yield();
  delay(1000);
  display.clearDisplay();
  displayName("BRIAN", "LOUGH");
  timeClient.begin();
  delay(1000);
  timeClient.forceUpdate();
  Serial.println(timeClient.getEpochTime());
  delay(3000);
}

void displayText(String text, int yPos) {
  int16_t  x1, y1;
  uint16_t w, h;
  display.setTextSize(2);
  char charBuf[text.length() + 1];
  text.toCharArray(charBuf, text.length() + 1);
  display.getTextBounds(charBuf, 0, yPos, &x1, &y1, &w, &h);
  int startingX = 33 - (w / 2);
  if (startingX < 0) {
    display.setTextSize(1);
    display.getTextBounds(charBuf, 0, yPos, &x1, &y1, &w, &h);
    startingX = 33 - (w / 2);
  }
  display.setCursor(startingX, yPos);
  Serial.println(startingX);
  Serial.println(yPos);
  display.print(text);
}

struct timeLeft_t {
    uint8_t Seconds, Minutes, Hours;
    uint16_t Days;
  } timeLeft;


// From this example 
// https://pastebin.com/sfEjA94n
void calculateTimeLeft(){

  //Serial.println((uint32_t)(makeTime(finishTime)));
  //Serial.println(timeClient.getEpochTime());
  uint32_t difference = (uint32_t)(makeTime(finishTime) - timeClient.getEpochTime());

  timeLeft.Seconds = difference % 60;
  difference /= 60; // now it is minutes
 
  timeLeft.Minutes = difference % 60;
  difference /= 60; // now it is hours
 
  timeLeft.Hours = difference % 24;
  difference /= 24; // now it is days
 
  timeLeft.Days = difference;
}

void updateScreen(long amountPledged, long numBackers) {
  calculateTimeLeft();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(myGREEN);
  display.setCursor(1, 1);
  display.print(projectName);
  display.setTextColor(myCYAN);
  display.setCursor(1, 9); 
  String pledged = "$" + String(amountPledged) + " of";
  String goal = String(target/1000) + "K goal";
  display.print(pledged);
  display.setTextColor(myMAGENTA);
  display.setCursor(1, 17);
  display.print(goal);
  display.setTextColor(myBLUE);
  display.setCursor(1, 25);
  String daysLeft = String(timeLeft.Days) + "d left";
  display.print(daysLeft);
}

void displayName(String first, String last)
{
  display.setTextColor(myCYAN);
  displayText(first, 1);
  display.setTextColor(myMAGENTA);
  displayText(last, 17);
}

void loop() {
  if (millis() > ntpUpdateDueTime)  {
    timeClient.update();
    ntpUpdateDueTime = millis() + delayBetweenNTPUpdates;
  }
  
  if (millis() > requestDueTime)  {
    if (kss.getProjectStats(KS_USER, KS_PROJECT))
    {
      Serial.println("---------Project Stats---------");
      Serial.print("Id: ");
      Serial.println(kss.projectStats.id);
      Serial.print("State Changed At: ");
      Serial.println(kss.projectStats.state_changed_at);
      Serial.print("State: ");
      Serial.println(kss.projectStats.state);
      Serial.print("Backers Count: ");
      Serial.println(kss.projectStats.backers_count);

      backerCount = kss.projectStats.backers_count;

      Serial.print("Pledged: $");
      Serial.println(kss.projectStats.pledged); //This comes back in the KS user's currency
      pledged = kss.projectStats.pledged;

      Serial.print("Comments Count: ");
      Serial.println(kss.projectStats.comments_count);
      Serial.println("------------------------");

      updateScreen(pledged, backerCount);

    }

    requestDueTime = millis() + delayBetweenRequests;
  }
}
