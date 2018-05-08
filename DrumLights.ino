#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include <WiFi.h>
#include <DNSServer.h>

#define LIS3DH_CS 21
// hardware SPI
Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS);

#define PIN            12
#define NUMPIXELS      68
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

Preferences preferences;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WiFiServer server(80);

int Brightness = 20;
bool Flash = false;

unsigned long DemoTime;
unsigned long DemoPeriod;

int TimeOut = 30000;

void setup() {
  //Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("DrumLights", "HelloRockview");

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  server.begin();

  preferences.begin("dl", false);

  unsigned int iSense = preferences.getUInt("sense", 50);

  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    //Serial.println("Couldnt start");
    while (1);
  }
  
  lis.setRange(LIS3DH_RANGE_16_G);   // 2, 4, 8 or 16 G!

  // 0 = turn off click detection & interrupt
  // 1 = single click only interrupt output
  // 2 = double click only interrupt output, detect single click
  // Adjust threshhold, higher numbers are less sensitive
  lis.setClick(1, iSense);

  unsigned int iChecked = preferences.getUInt("checked", 0);
  if (iChecked = 1){
    Flash = true;
  }
  Brightness = preferences.getUInt("brightness", 20);

  pixels.begin();
  randomSeed(analogRead(2));
  delay(100);
}

void loop() {
  dnsServer.processNextRequest();
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.print(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            String myResponse = createResponse();
            client.print(myResponse);
            break;
          } else {
            if(currentLine.substring(0,6)== "GET /?"){
              if(currentLine.indexOf("flash=on")>0){
                preferences.putUInt("checked", 1);
                Flash = true;
              }else{
                preferences.putUInt("checked", 0);
                Flash = false;
              }
              if(currentLine.indexOf("sensitivity=")>0){
                if(currentLine.lastIndexOf("&") < currentLine.indexOf("sensitivity=")){
                  int iSense = currentLine.substring(currentLine.indexOf("sensitivity=")+12,currentLine.indexOf(" HTTP/")).toInt();
                  preferences.putUInt("sense", iSense);
                  lis.setClick(1, iSense);
                  //Serial.println("Writing Response");
                }
              }
              if(currentLine.indexOf("brightness=")>0){
                  int iBrightness = currentLine.substring(currentLine.indexOf("brightness=")+11,currentLine.indexOf("&sensitivity=")).toInt();
                  //Serial.println(iBrightness);
                  preferences.putUInt("brightness", iBrightness);
                  Brightness = iBrightness;
                  //Serial.println("Writing Response");
              }
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    }else{
        uint8_t click = lis.getClick();
        if(millis()-DemoTime >= TimeOut){;
           demoMode();
        }
        if (click == 0) return;
        if (! (click & 0x30)) return;
        if (click & 0x10){ 
          //Serial.println("single click");
          int r = random(0,255);
          int g = random(0,255);
          int b = random (0,255);
          if(Flash){
            flashLEDs();
          }

          changeAllLEDs(r,g,b,Brightness);
          
          DemoTime = millis();
        } 
        delay(100);
        return;
    }
    
  }

String createResponse(){
  String sChecked = "";

  unsigned int iChecked = preferences.getUInt("checked", 0);
  unsigned int iSense = preferences.getUInt("sense", 50);
  unsigned int iBrightness = preferences.getUInt("brightness", 20);

  if(iChecked !=0){
    sChecked = "checked";
  }

  
  String sOut = ""
    "<!DOCTYPE html><html><head><title>Drum Lights</title></head><body>"
  "<h1 style=""color:CHARTREUSE"">Drum Lights</h1><p>";
//sOut += iChecked;
sOut +="</p><form action=""./"">"
"  <h3>Flash:</h3>"
"  <div class=""onoffswitch"">"
"    <input type=""checkbox"" name=""flash"" class=""onoffswitch-checkbox"" id=""myonoffswitch"" ";

sOut += sChecked;
sOut += ">"
"    <label class=""onoffswitch-label"" for=""myonoffswitch"">"
"        <span class=""onoffswitch-inner""></span>"
"        <span class=""onoffswitch-switch""></span>"
"    </label>"
"  </div>"
"<br>"
"  <h3>Brightness:</h3>"
"  <input type=""range"" name=""brightness"" class=""slider"" value=""";
sOut += iBrightness;
sOut += """ min=""0"" max=""80"" step=""1"">"
"  <h3>Sensitivity:</h3>"
"  <input type=""range"" name=""sensitivity"" class=""slider"" value=""";
sOut += iSense;
sOut += """ min=""0"" max=""100"" step=""1"">"
"  <br><br><br>"
"  <input class=""button button1"" type=""submit"" value=""Submit"">"
"</form>"
  "</body></html>";

  return sOut;

}

void flashLEDs(){
  
  pixels.setBrightness(100);
  
  for(int i=0;i<NUMPIXELS;i++){
    
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(255,255,255)); // Moderately bright green color.
  }

  pixels.show(); // This sends the updated pixel color to the hardware.

  //Serial.println("Flash");
  delay(100);
}

void changeAllLEDs(int r,int g,int b,int bright){
  pixels.setBrightness(bright);
  
  for(int i=0;i<NUMPIXELS;i++){
    
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(r,g,b)); // Moderately bright green color.
  }

  pixels.show(); // This sends the updated pixel color to the hardware.
}

void demoMode(){

  if(millis()-DemoPeriod >= 5000){
      
      int DemoR = random(0,255);
      int DemoG = random(0,255);
      int DemoB = random (0,255);

      changeAllLEDs(DemoR, DemoG, DemoB, Brightness);

      DemoPeriod = millis();    
  }
}
