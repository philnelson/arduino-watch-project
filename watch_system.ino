/*
   This is all HEAVILY based on the TimeSerial.pde file from the excellent Time project
   available here: https://github.com/PaulStoffregen/Time none of this would've been
   possible without their work.
*/

#include <TimeLib.h>
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST 7 // ASCII bell character requests a time sync message
#define VBATPIN A9
#define BUZZERPIN A5
#define TIME_MSG_LEN 10

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

  MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
  MODE_LED_BEHAVIOUR        LED activity, valid options are
                            "DISABLE" or "MODE" or "BLEUART" or
                            "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE      1
#define MINIMUM_FIRMWARE_VERSION    "0.8.0"
#define MODE_LED_BEHAVIOUR          "SPI"
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
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 5
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

char* APP_MODE = "boot";

Adafruit_BLEBattery battery(ble);

unsigned long TIME_SINCE_START;

// the setup function runs once when you press reset or power the board
void setup() {
  while (!Serial);  // required for Flora & Micro

  delay(500);

  Serial.begin(9600);

  pinMode(13, OUTPUT);
  pinMode(BUZZERPIN, OUTPUT);

  setSyncProvider( requestSync);  //set function to call when sync required
  setSyncInterval(1000);
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

  /* Wait for connection */
  while (! ble.isConnected()) {
    APP_MODE = "unsynced";
    delay(100);
  }

  APP_MODE = "face1";

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
  //display.display();

  //display.clearDisplay();

  // Enable Battery service and reset Bluefruit
  battery.begin(true);
}

// the loop function runs over and over again forever
void loop() {
  display.clearDisplay();

  digitalClockDisplay();

  //  Serial.println("Printing to display");
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  int hourDigit = hour();

  char* minuteDisplay;
  char* hourDisplay;

  if (minute() < 15) {
    minuteDisplay = "just";
  }

  if (minute() > 15) {
    minuteDisplay = "1/4 past";
  }

  if (minute() > 30) {
    minuteDisplay = "1/2 past";
  }

  if (minute() > 45) {
    minuteDisplay = "1/4 to";
    hourDigit = hour() + 1;

    if (hourDigit == 13) {
      hourDigit = 12;
    }
  }

  display.println(minuteDisplay);
  //Serial.println(minuteDisplay);

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

  display.print(hourDisplay);
  //display.println(TEXT_STRING);

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

  display.setCursor(60, 54);

  if (timeStatus() == timeSet) {
    //display.print("set");
  } else {
    //display.print("unset");
  }

  display.print(APP_MODE);

  display.display();

  TIME_SINCE_START = millis();

  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  battery.update(measuredvbat);

  processSyncMessage();

  delay(100);

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
  if (Serial.find(TIME_HEADER)) {
    pctime = Serial.parseInt();
    if ( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
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
bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while ( (!Serial.available()) && !timeout.expired() ) {
    delay(1);
  }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count = 0;
  do
  {
    count += Serial.readBytes(buffer + count, maxSize);
    delay(2);
  } while ( (count < maxSize) && (Serial.available()) );

  return true;
}
