/*
 * This is all HEAVILY based on the TimeSerial.pde file from the excellent Time project
 * available here: https://github.com/PaulStoffregen/Time none of this would've been
 * possible without their work.
 */

#include <TimeLib.h>
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST 7 // ASCII bell character requests a time sync message

#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEBattery.h"
#include "BluefruitConfig.h"

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE     Perform a factory reset when running this sketch
   
                            Enabling this will put your Bluefruit LE module
                            in a 'known good' state and clear any config
                            data set in previous sketches or projects, so
                            running this at least once is a good idea.
   
                            When deploying your project, however, you will
                            want to disable factory reset by setting this
                            value to 0.  If you are making changes to your
                            Bluefruit LE device via AT commands, and those
                            changes aren't persisting across resets, this
                            is the reason why.  Factory reset will erase
                            the non-volatile memory where config data is
                            stored, setting it back to factory default
                            values.
       
                            Some sketches that require you to bond to a
                            central device (HID mouse, keyboard, etc.)
                            won't work at all with this feature enabled
                            since the factory reset will clear all of the
                            bonding data stored on the chip, meaning the
                            central device won't be able to reconnect.
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE      1
/*=========================================================================*/


// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

int value = 100;

#define SPKR 0

// the setup function runs once when you press reset or power the board
void setup() {
  while (!Serial);  // required for Flora & Micro
  delay(500);

  pinMode(SPKR, OUTPUT); //set the speaker as output

  Serial.begin(115200);

  pinMode(13, OUTPUT);
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for time sync message");

  Serial.println(F("Adafruit Bluefruit AT Command Example"));
  Serial.println(F("-------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
}

// the loop function runs over and over again forever
void loop() {
  
  digitalClockDisplay();

  if (Serial.available()) {
    processSyncMessage();
  }

  // Display command prompt
  Serial.print(F("AT > "));

  if (timeStatus() == timeSet) {
    digitalWrite(13, LOW);  // LED off if needs refresh
    delay(1000);
  } else {
    digitalWrite(13, HIGH);  // LED off if needs refresh
    delay(100);
    digitalWrite(13, LOW);  // LED off if needs refresh
    delay(100);
  }

/*
   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(1000);                     
   digitalWrite(SPKR, LOW);
   delay(100);
   
   digitalWrite(SPKR, HIGH);
   delay(1000);                     
   digitalWrite(SPKR, LOW);
   delay(100);
   
   digitalWrite(SPKR, HIGH);
   delay(1000);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);

   digitalWrite(SPKR, HIGH);
   delay(100);                     
   digitalWrite(SPKR, LOW);
   delay(100);
   */

  // Check for user input and echo it back if anything was found
  //char command[BUFSIZE+1];
  //getUserInput(command, BUFSIZE);

  // Send command
  //ble.println(command);
  //ble.println("testing");

  // Check response status
  //ble.waitForOK();
  
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(dayStr(weekday()));
  Serial.print(" ");
  Serial.print(monthStr(month()));
  Serial.print(" ");
  Serial.print(day());
  //Serial.print(" ");
  //Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1529177175; // Now

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

/**************************************************************************/
/*!
    @brief  Checks for user input (via the Serial Monitor)
*/
/**************************************************************************/
void getUserInput(char buffer[], uint8_t maxSize)
{
  memset(buffer, 0, maxSize);
  while( Serial.available() == 0 ) {
    delay(1);
  }

  uint8_t count=0;

  do
  {
    count += Serial.readBytes(buffer+count, maxSize);
    delay(2);
  } while( (count < maxSize) && !(Serial.available() == 0) );
}