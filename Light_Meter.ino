/*
MIT License

Copyright (c) 2022 Nolan Sornson nolansornson@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>
#include <Encoder.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"


/*
 * Constant Pin Mapping
 */
#define TFT_CS        10
#define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8
const int buttonPin = 7;
const int encoderButtonPin = 4;

const int iso_button_pin = 3;
const int aperture_button_pin = 2;
const int shutter_button_pin = 1;

/*
 * Instantiate Objects
 */
// For 1.14", 1.3", 1.54", 1.69", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
Encoder enc = Encoder(5,6);


/*
 * Create Global Variables
 */
float lux;
long oldPosition;
int buttonState;
int encoderButtonState;


/*
 * Create Arrays and Sizes of Arrays for Camera Settings
 */
String testArray[] = {"zero", "one", "two", "three", "four", "five", "six"};
int testArraySize = 7;

long aperture[] = {5.3, 5.6, 6.3, 7.1, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 25, 29, 32, 36};
long iso[] = {100, 200, 400, 800, 1600, 3200, 6400, 12800};
long shutter_value[] = {60.0, 30.0, 25.0, 20.0, 15.0, 13.0, 10.0, 8.0, 6.0, 5.0, 4.0, 3.0, 
                      2.5, 2, 1.6, 1.3, 1, 1/1.3, 1/1.6, 1/2.0, 1/2.5, 1/3.0, 1/4.0, 1/5.0, 
                      1/6.0, 1/8.0, 1/10.0, 1/13.0, 1/15.0, 1/20.0, 1/25.0, 1/30.0, 1/40.0, 
                      1/50.0, 1/60.0, 1/80.0, 1/100.0, 1/125.0, 1/160.0, 1/200.0, 1/250.0,
                      1/320.0, 1/400.0, 1/500.0, 1/640.0, 1/800.0, 1/1000.0, 1/1250.0,
                      1/1600.0, 1/2000.0, 1/2500.0, 1/3200.0, 1/4000.0};
                      
String shutter_string[] = {"60.0", "30.0", "25.0", "20.0", "15.0", "13.0", "10.0", "8.0", "6.0", "5.0", "4.0", "3.0", 
                      "2.5", "2", "1.6", "1.3", "1", "1/1.3", "1/1.6", "1/2.0", "1/2.5", "1/3.0", "1/4.0", "1/5.0", 
                      "1/6.0", "1/8.0", "1/10.0", "1/13.0", "1/15.0", "1/20.0", "1/25.0", "1/30.0", "1/40.0", 
                      "1/50.0", "1/60.0", "1/80.0", "1/100.0", "1/125.0", "1/160.0", "1/200.0", "1/250.0",
                      "1/320.0", "1/400.0", "1/500.0", "1/640.0", "1/800.0", "1/1000.0", "1/1250.0",
                      "1/1600.0", "1/2000.0", "1/2500.0", "1/3200.0", "1/4000.0"};

/*
 * Init method, only runs once. 
 */
void setup() {
  //Start serial output
  Serial.begin(9600);

  //Initialize Screen
  tft.init(240,240); //ST7789 240x240
  Serial.println(F("Initialized"));
  tft.fillScreen(ST77XX_BLACK);

  //Initialize Sensor
  Serial.println(F("Starting Adafruit TSL2591 Test!"));
  if (tsl.begin()) {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }
  displaySensorDetails(); //Display some basic information on this sensor
  configureSensor(); //Configure this sensor

  //set initial conditions for push button
  buttonState = 0;
  pinMode(buttonPin, INPUT);

  //Set initial conditions for encoder
  encoderButtonState = 1;
  pinMode(buttonPin, INPUT);

  //Initial Variables to prevent errors
  lux = -999;
  oldPosition = -999;
}

/*
 * Main Loop
 */
void loop() {

  //These are guarunteed to happen every loop
  long newPosition = enc.read();
  buttonState = digitalRead(buttonPin); 

  
  if(newPosition != oldPosition)
  {
    oldPosition = newPosition;
    if(newPosition % 4 == 0)  //Only update when reach detents
      updateTFT(lux, newPosition/4, testArray, testArraySize); //Display count of detents, not raw encoder value
  }

  if(buttonState == HIGH)
  {
    lux = advancedRead();
    updateTFT(lux, newPosition, testArray, testArraySize);
  }
}

void updateTFT(float lux, long encoderPosition, String arr[], int arrSize){

  int header_indent = 10;
  int data_indent = 100;
  int top_indent = 10;
  int spacing = 40;
  int fontSize = 3;
  
  tft.fillScreen(ST77XX_BLACK);
  
  String luxStr = String(lux);
  drawTextGeneric("Lux:", ST77XX_WHITE, header_indent, top_indent + 0*spacing, fontSize);
  drawTextGeneric(&luxStr[0], ST77XX_WHITE, data_indent, top_indent + 0*spacing, fontSize);

  String encoderPosStr = String(encoderPosition);
  drawTextGeneric("Enc:", ST77XX_WHITE, header_indent, top_indent + 1*spacing, fontSize);
  drawTextGeneric(&encoderPosStr[0], ST77XX_WHITE, data_indent, top_indent + 1*spacing, fontSize);

  if(encoderPosition >= arrSize - 1) {
    encoderPosition = arrSize - 1;
  }
  if(encoderPosition <= 0) {
    encoderPosition = 0;
  }
  
  drawTextGeneric("Arr:", ST77XX_WHITE, header_indent, top_indent + 2*spacing, fontSize);
  drawTextGeneric(&(arr[encoderPosition])[0], ST77XX_WHITE, data_indent, top_indent + 2*spacing, fontSize);
}

void drawTextGeneric(char *text, uint16_t color, int x, int y, int fontSize) {
  tft.setCursor(x, y);
  tft.setTextSize(fontSize);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:   ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:    ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:    ")); Serial.print(sensor.max_value); Serial.println(F(" lux"));
  Serial.print  (F("Min Value:    ")); Serial.print(sensor.min_value); Serial.println(F(" lux"));
  Serial.print  (F("Resolution:   ")); Serial.print(sensor.resolution, 4); Serial.println(F(" lux"));  
  Serial.println(F("------------------------------------"));
  Serial.println(F(""));
  delay(500);
}

void configureSensor(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* Display the gain and integration time for reference sake */  
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();
  switch(gain)
  {
    case TSL2591_GAIN_LOW:
      Serial.println(F("1x (Low)"));
      break;
    case TSL2591_GAIN_MED:
      Serial.println(F("25x (Medium)"));
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println(F("428x (High)"));
      break;
    case TSL2591_GAIN_MAX:
      Serial.println(F("9876x (Max)"));
      break;
  }
  Serial.print  (F("Timing:       "));
  Serial.print((tsl.getTiming() + 1) * 100, DEC); 
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------"));
  Serial.println(F(""));
}

float advancedRead(void)
{
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full, visible;

  long time = millis();
  ir = lum >> 16;
  full = lum & 0xFFFF;
  visible = full - ir;
  float lux = tsl.calculateLux(full, ir);

  Serial.print(F("[ ")); Serial.print(time); Serial.print(F(" ms ] "));
  Serial.print(F("IR: ")); Serial.print(ir);  Serial.print(F("  "));
  Serial.print(F("Full: ")); Serial.print(full); Serial.print(F("  "));
  Serial.print(F("Visible: ")); Serial.print(visible); Serial.print(F("  "));
  Serial.print(F("Lux: ")); Serial.println(lux, 6);

  return lux;
  
}
