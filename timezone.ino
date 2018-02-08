#include <WiFiManager.h>

/*
 *    timezone.ino
 *    Written By Spencer Perry
 *    5/3/2016
 * 
 */


#include <Wire.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

const time_t EPOCH_GRAD = 1559174400;
const time_t EPOCH_ANN = 1462233600;

#define DISPLAY_ADDRESS   0x70

// Create Display Object
Adafruit_7segment ClockDisplay = Adafruit_7segment();

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

// const int timeZone = 1;     // Central European Time
const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


WiFiUDP Udp;

unsigned int localPort = 8888;  // local port to listen for UDP packets

bool TIME_24_HOUR = false; 

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup()
{
  pinMode(2,INPUT_PULLUP);
  attachInterrupt(2, OnButtonPress, RISING);

  // WiFi.disconnect();
  Wire.begin(4,5);

  Serial.begin(9600);
  ClockDisplay.begin(DISPLAY_ADDRESS);
  
  WiFiManager wifiManager;
  //first parameter is name of access point, second is the password
  wifiManager.autoConnect("4F1NGERFREDDY", "nognomes");
 
  
  Udp.begin(localPort);
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

time_t prevDisplay = 0; // when the digital clock was displayed
volatile int mode = 1;

void loop()
{
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed    
      prevDisplay = now();
      CalcDisplay(mode);
      ClockDisplay.writeDisplay();
    }
  }
}

void CalcDisplay(int mode) {
  if(mode == 0) {
      digitalClockDisplay();
  }
  else if (mode == 1) {
      int diff = ((EPOCH_GRAD - now()) /60/ 60/24); 
      ClockDisplay.print(diff, DEC); 
  }
  else if (mode == 2)  {
      int diff = (-1 * (EPOCH_ANN - now()) /60/ 60/24);
      Serial.println(diff);
      ClockDisplay.print(diff, DEC);
  }
  else {
    ClockDisplay.printError();
  }
}

void digitalClockDisplay()
{
  // digital clock display of the time
  int hours = hour();
  int minutes =minute();
  int seconds = (second());
  int displayValue = hours*100 + minutes;

    // Do 24 hour to 12 hour format conversion when required.
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (hours > 12) {
      displayValue -= 1200;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayValue += 1200;
    }
  }
  
  ClockDisplay.print(displayValue, DEC);
  ClockDisplay.drawColon(true);
  ClockDisplay.writeDisplay();

}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  getNtpTime();
  return 0; // return 0 if unable to get the time
}

void OnButtonPress() {
  Serial.print("Mode Changed");
  mode = (mode + digitalRead(2)) % 3;
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
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
