/*
   This is all HEAVILY based on the TimeSerial.pde file from the excellent Time project
   available here: https://github.com/PaulStoffregen/Time none of this would've been
   possible without their work.
*/

#include <TimeLib.h>
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define FACE_HEADER  "F"   // Header tag for serial command message
#define TIME_REQUEST 7 // ASCII bell character requests a time sync message
#define VBATPIN A9
#define TIME_MSG_LEN 11

#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
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

  MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
  MODE_LED_BEHAVIOUR        LED activity, valid options are
                            "DISABLE" or "MODE" or "BLEUART" or
                            "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE      1
#define MINIMUM_FIRMWARE_VERSION    "0.8.0"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BLE_READPACKET_TIMEOUT 250
/*=========================================================================*/


// Create the bluefruit object, either software serial...uncomment these lines
/*
  SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

  Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
//Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

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

char* TEXT_STRING = "Hi.";

int value = 100;

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   6
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 5
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

char* APP_MODE = "boot";

unsigned long TIME_SINCE_START;

int incomingByte = 0;

// the setup function runs once when you press reset or power the board
void setup() {

  //while (!Serial);  // required for Flora & Micro

  delay(500);

  Serial.begin(9600);

  pinMode(13, OUTPUT);
  pinMode(VBATPIN, OUTPUT);

  setSyncProvider( requestSync);  //set function to call when sync required
  setSyncInterval(10000);
  Serial.println("Waiting for time sync message");

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
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  ble.setMode(BLUEFRUIT_MODE_DATA);

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }

  // SCREEN INIT
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();

  //display.clearDisplay();

  APP_MODE = "unsynced";

  delay(5000);
}

// the loop function runs over and over again forever
void loop() {

  display.clearDisplay();

  digitalClockDisplay();

  if (APP_MODE == "unsynced") {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    if (! ble.isConnected()) {
      display.println("Hi!");
    }
    else {
      display.println("Connected.");
    }

    display.setCursor(0, 20);
    display.setTextSize(1);
    if (! ble.isConnected()) {
      display.println("Connect via Bluetooth to set the time.");
    }
    else {
      display.println("Connected via Bluetooth... hang tight.");
    }

  }

  if (APP_MODE == "face1") {
    displayFaceOne();
  }

  if (APP_MODE == "face2") {
    displayFaceTwo();
  }

  display.display();

  TIME_SINCE_START = millis() / 1000;

  //ble.print(TIME_SINCE_START);

  if (APP_MODE == "unsynced") {
    processSyncMessage();
  }
  else
  {
    // Go into low-power mode
    processFaceMessage();
    delay(400);
  }

}

void digitalClockDisplay() {
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

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1529743356; // Now

  //setTime(DEFAULT_TIME);
  if (Serial.available() > 0) {
    if (Serial.find(TIME_HEADER)) {
      Serial.println("Setting time via Serial");
      pctime = Serial.parseInt();
      if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
        setTime(pctime); // Sync Arduino clock to the time received on the serial port
        adjustTime(-25200);
        // Turn off BLE light
        ble.sendCommandCheckOK("AT+HWModeLED=DISABLE");
        APP_MODE = "face1";
      }
    }
  }

  if (ble.available() > 0) {
    if (ble.find(TIME_HEADER)) {
      Serial.println("Setting time via BLE");
      pctime = ble.parseInt();
      if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
        setTime(pctime); // Sync Arduino clock to the time received on the serial port
        adjustTime(-25200);
        // Turn off BLE light
        ble.sendCommandCheckOK("AT+HWModeLED=DISABLE");
        APP_MODE = "face1";
      }
    }

    //String ble_str = (char*)ble.buffer;

    ble.waitForOK();

  }

}

void processFaceMessage() {

  if (ble.available() > 0) {
    String ble_input = ble.readString();

    Serial.println(ble_input);

    ble_input.trim();

    if (ble_input == "Fface1") {
      Serial.println("face1");
      APP_MODE = "face1";
    }

    if (ble_input == "Fface2") {
      Serial.println("face2");
      APP_MODE = "face2";
    }
    
    ble.waitForOK();
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);
  return 0; // the time will be sent later in response to serial mesg
}

void displayFaceOne()
{
  //  Serial.println("Printing to display");
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  int hourDigit = hourFormat12();

  char* minuteDisplay;
  char* hourDisplay;

  if (minute() == 0) {
    minuteDisplay = "precisely";
  }

  if (minute() < 3) {
    minuteDisplay = "just after";
  }

  if (minute() >= 3) {
    minuteDisplay = "5 past";
  }

  if (minute() >= 8) {
    minuteDisplay = "10 past";
  }

  if (minute() >= 13) {
    minuteDisplay = "1/4 past";
  }

  if (minute() >= 18) {
    minuteDisplay = "20 past";
  }

  if (minute() >= 23) {
    minuteDisplay = "25 past";
  }

  if (minute() >= 28) {
    minuteDisplay = "1/2 past";
  }

  if (minute() >= 33) {
    minuteDisplay = "25 to";
    hourDigit = hourFormat12() + 1;

    if (hourDigit == 13) {
      hourDigit = 12;
    }
  }

  if (minute() >= 38) {
    minuteDisplay = "20 to";
  }

  if (minute() >= 43) {
    minuteDisplay = "1/4 to";
  }

  if (minute() >= 48) {
    minuteDisplay = "10 to";
  }

  if (minute() >= 53) {
    minuteDisplay = "almost";
  }

  if (hourDigit == 0) {
    hourDisplay = "twelve";
    //Serial.print("twelve");
  }

  if (hourDigit == 1) {
    hourDisplay = "one";
    //Serial.print("one");
  }

  if (hourDigit == 2) {
    hourDisplay = "two";
    //Serial.print("two");
  }

  if (hourDigit == 3) {
    hourDisplay = "three";
    //Serial.print("three");
  }

  if (hourDigit == 4) {
    hourDisplay = "four";
    //Serial.print("four");
  }

  if (hourDigit == 5) {
    hourDisplay = "five";
    //Serial.print("five");
  }

  if (hourDigit == 6) {
    hourDisplay = "six";
    //Serial.print("six");
  }

  if (hourDigit == 7) {
    hourDisplay = "seven";
    //Serial.print("seven");
  }

  if (hourDigit == 8) {
    hourDisplay = "eight";
    //Serial.print("eight");
  }

  if (hourDigit == 9) {
    hourDisplay = "nine";
    //Serial.print("nine");
  }

  if (hourDigit == 10) {
    hourDisplay = "ten";
    //Serial.print("ten");
  }

  if (hourDigit == 11) {
    hourDisplay = "eleven";
    //Serial.print("eleven");
  }

  if (hourDigit == 12) {
    hourDisplay = "twelve";
    //Serial.print("twelve");
  }

  if (hourDisplay == "four" && (minute(34) || minute(35))) {
    display.println("twenty-five or six to four");
  }
  else {
    display.println(minuteDisplay);
    display.print(hourDisplay);
  }

  if (isAM()) {
    display.println(" am");
    //Serial.println(" in the morning");
  } else {
    display.println(" pm");
    //Serial.println(" at night");
  }

  //display.setTextSize(2);
  //display.println(second());

  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(monthStr(month()));
  display.print(" ");
  display.print(day());

  display.setCursor(50, 54);

  if (timeStatus() == timeSet) {
    //display.print("no sync");
  } else {
    //display.print("sync");
  }

  display.setCursor(80, 54);

  display.print(APP_MODE);
}

void displayFaceTwo() {
  display.setCursor(0, 0);
  display.setTextSize(3);
  display.print(hour());
  display.print(":");
  display.println(minute());
  display.print(APP_MODE);
}

