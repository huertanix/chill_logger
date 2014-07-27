#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed
#define WLAN_SSID       "NYCR24"           // cannot be longer than 32 characters!
#define WLAN_PASS       "clubmate"
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.
uint32_t ip; // IP address
int sensorPin = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println(F("CC3000 Activated.\n")); 
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  ip = 0;
  ip = cc3000.IP2U32(162, 243, 120, 32); // DO instance
  // cc3000.printIPdotsRev(ip);
}

void loop() {
  int temp_reading = analogRead(sensorPin);
  float voltage = temp_reading * 3.3;
  voltage /= 1024.0;
  // print out the voltage
  Serial.print(voltage); Serial.println(" volts");
  // now print out the temperature
  //float temp_c = (voltage - 0.5) * 100; //converting from 10 mv per degree wit 500 mV offset
                                        //to degrees ((voltage - 500mV) times 100)
  float temp_c = (voltage - 0.33) * 100; // using 3.33v line since 5v is being used by wifi shield
  Serial.print(temp_c); Serial.println(" degrees C");
  // now convert to Fahrenheit
  float temp_f = (temp_c * 9.0 / 5.0) + 32.0;
  Serial.print(temp_f); Serial.println(" degrees F");

  char temp_chars[5];
  dtostrf(temp_f,1,2,temp_chars);
  String temp_string;
  temp_string = String(temp_chars);
  String parms;
  
  //parms = "/?TEST=0A\0";// + temp_string; // "works" as in it can connect
  parms = "/?temperature=" + temp_string + "\A\0";
  //char full_parms_char[parms.length()];
  //full_parms_char = parms;
  // Ugh. Burn this language to a cinder and replace it all with JavaScript
  //char full_parms_char[parms.length()];
  int parms_length = parms.length();
  char full_parms_char[parms_length +1];
  parms.toCharArray(full_parms_char, parms_length);
  //Serial.println(full_parms_char);
  //Serial.println(parms_length);
  
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 8000);
  if (www.connected()) {
    Serial.println(F("Sending temperature reading to The Cloud..."));
    www.fastrprint(F("GET "));
    www.fastrprint(full_parms_char);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    //www.fastrprint(F("Host: ")); www.fastrprint("www.flyingsexsnak.es"); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
    Serial.println(F("Sent."));
  } else {
    Serial.println(F("Connection failed"));
    return;
  }
  
  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  
  www.close();
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  Serial.println(F("\n\nDisconnecting..."));
  cc3000.disconnect();
  Serial.println(F("Disconnected."));
  
  delay(60000); // Sample once every minute
}
