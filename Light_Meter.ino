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
const int iso_button_pin = 4;
const int aperture_button_pin = 3;
const int shutter_button_pin = 2;
const float C = 12.5; //incident light meter calibration constant

/*
 * Instantiate Objects
 */
// For 1.14", 1.3", 1.54", 1.69", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
Encoder iso_enc = Encoder(5,6);
Encoder aperture_enc = Encoder(14,15);
Encoder shutter_enc = Encoder(20,21);


/*
 * Create Global Variables
 */
float lux;
int button_state;
long iso_old_pos, aperture_old_pos, shutter_old_pos;
int iso_button_state, aperture_button_state, shutter_button_state;
int calculated;

/*
 * Create Arrays and Sizes of Arrays for Camera Settings
 */
float aperture[] = {5.3, 5.6, 6.3, 7.1, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 25, 29, 32, 36};
int aperture_array_size = 18;

float iso[] = {100, 200, 400, 800, 1600, 3200, 6400, 12800};
int iso_array_size = 8;

float shutter[] = {60.0, 30.0, 25.0, 20.0, 15.0, 13.0, 10.0, 8.0, 6.0, 5.0, 4.0, 3.0, 
                      2.5, 2, 1.6, 1.3, 1.0, 1/1.3, 1/1.6, 1/2.0, 1/2.5, 1/3.0, 1/4.0, 1/5.0, 
                      1/6.0, 1/8.0, 1/10.0, 1/13.0, 1/15.0, 1/20.0, 1/25.0, 1/30.0, 1/40.0, 
                      1/50.0, 1/60.0, 1/80.0, 1/100.0, 1/125.0, 1/160.0, 1/200.0, 1/250.0,
                      1/320.0, 1/400.0, 1/500.0, 1/640.0, 1/800.0, 1/1000.0, 1/1250.0,
                      1/1600.0, 1/2000.0, 1/2500.0, 1/3200.0, 1/4000.0};                     
String shutter_string[] = {"60.0", "30.0", "25.0", "20.0", "15.0", "13.0", "10.0", "8.0", "6.0", "5.0", "4.0", "3.0", 
                      "2.5", "2", "1.6", "1.3", "1.0", "1/1.3", "1/1.6", "1/2", "1/2.5", "1/3", "1/4", "1/5", 
                      "1/6", "1/8", "1/10", "1/13", "1/15", "1/20", "1/25", "1/30", "1/40", 
                      "1/50", "1/60", "1/80", "1/100", "1/125", "1/160", "1/200", "1/250",
                      "1/320", "1/400", "1/500", "1/640", "1/800", "1/1000", "1/1250",
                      "1/1600", "1/2000", "1/2500", "1/3200", "1/4000"};
int shutter_array_size = 53;

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
  button_state = 0;
  pinMode(buttonPin, INPUT);

  //Set initial conditions for encoder buttons
  iso_button_state = 1;
  pinMode(iso_button_pin, INPUT_PULLUP);

  aperture_button_state = 1;
  pinMode(aperture_button_pin, INPUT_PULLUP);

  shutter_button_state = 1;
  pinMode(shutter_button_pin, INPUT_PULLUP);

  //Initial Variables to prevent errors
  lux = -999;
  iso_old_pos = -999;
  aperture_old_pos = -999;
  shutter_old_pos = -999;
  calculated = 0;
}

/*
 * Main Loop
 */
void loop() {

  //These are guarunteed to happen every loop
  long iso_new_pos, aperture_new_pos, shutter_new_pos;
  iso_new_pos = iso_enc.read();
  aperture_new_pos = aperture_enc.read();
  shutter_new_pos = shutter_enc.read();
  
  button_state = digitalRead(buttonPin); 
  iso_button_state = digitalRead(iso_button_pin);
  aperture_button_state = digitalRead(aperture_button_pin);
  shutter_button_state = digitalRead(shutter_button_pin);


  //set calculated
  if(iso_button_state == 0)
    calculated = 0;
  if(aperture_button_state == 0)
    calculated = 1;
  if(shutter_button_state == 0)
    calculated = 2;

  //Check for updates with ISO encoder
  if(iso_new_pos != iso_old_pos && calculated != 0)
  {
    iso_old_pos = iso_new_pos;
    if(iso_new_pos % 4 == 0)  //Only update when reach detents
      updateTFT(lux, iso_new_pos/4, aperture_new_pos/4, shutter_new_pos/4, calculated); //Display count of detents, not raw encoder value
  }

  //Check for updates with aperture encoder
  if(aperture_new_pos != aperture_old_pos && calculated != 1)
  {
    aperture_old_pos = aperture_new_pos;
    if(aperture_new_pos % 4 == 0)  //Only update when reach detents
      updateTFT(lux, iso_new_pos/4, aperture_new_pos/4, shutter_new_pos/4, calculated); //Display count of detents, not raw encoder value
  }

  //Check for updates with shutter speed encoder
  if(shutter_new_pos != shutter_old_pos && calculated != 2)
  {
    shutter_old_pos = shutter_new_pos;
    if(shutter_new_pos % 4 == 0)  //Only update when reach detents
      updateTFT(lux, iso_new_pos/4, aperture_new_pos/4, shutter_new_pos/4, calculated); //Display count of detents, not raw encoder value
  }

  if(button_state == HIGH)
  {
    lux = advancedRead();
    
    /*
     * From https://en.wikipedia.org/wiki/Exposure_value the equation to calculate exposure value is as follows
     * N^2 / t = (E * S) / C
     * s.t.
     * N = aperture (f - number)
     * t = exposure time (aka shutter speed in seconds)
     * E = illuminance (lux)
     * S = iso arithmetic speed
     * C = incident-light meter calibration constant (assume 12.5)
     */
    int N;
    float t, S;
    float raw_iso = -1;
    float raw_aperture = -1;
    float raw_shutter = -1;
    switch(calculated) {
      case 0: //calculate iso
        N = aperture[aperture_new_pos/4];
        t = shutter[shutter_new_pos/4];
        raw_iso = (sq(N) * C) / (t * lux);
        iso_new_pos = closest_index(raw_iso, iso, iso_array_size) * 4;
        break;
      case 1: //calculate aperture
        t = shutter[shutter_new_pos/4];
        S = iso[iso_new_pos/4];
        raw_aperture = sqrt((t * lux * S) / C);
        aperture_new_pos = closest_index(raw_aperture, aperture, aperture_array_size) * 4;
        break;
      case 2: //calculate shutter
        N = aperture[aperture_new_pos/4];
        S = iso[iso_new_pos/4];
        raw_shutter = (sq(N) * C) / (lux * S);
        shutter_new_pos = closest_index(raw_shutter, shutter, shutter_array_size) * 4;
        break;        
    }
    updateTFT(lux, iso_new_pos/4, aperture_new_pos/4, shutter_new_pos/4, calculated); //Display count of detents, not raw encoder value

    Serial.print("Raw ISO: ");
    Serial.print(raw_iso);
    Serial.print(" Raw f#: ");
    Serial.print(raw_aperture);
    Serial.print(" Raw Shutter: ");
    Serial.println(raw_shutter);
  }

  /*
  Serial.print("ISO Button: ");
  Serial.print(iso_button_state);
  Serial.print(" Aperture Button: ");
  Serial.print(aperture_button_state);
  Serial.print(" Shutter Button: ");
  Serial.println(shutter_button_state);
  */
}

float closest_index(float num, float arr[], int arr_size){
  float curr = arr[0];
  int index = 0;
  for(int i = 0; i < arr_size - 1; i++)
  {
    if(abs(num - arr[i]) < abs(num - curr))
    {
      curr = arr[i];
      index = i;
    }
  }
  return index;
}

void updateTFT(float lux, int iso_pos, long aperture_pos, long shutter_pos, int calculated){

  int header_indent = 10;
  int data_indent = 100;
  int top_indent = 10;
  int spacing = 40;
  int fontSize = 3;
  
  tft.fillScreen(ST77XX_BLACK);
  
  String luxStr = String(lux);
  drawTextGeneric("Lux:", ST77XX_WHITE, header_indent, top_indent + 0*spacing, fontSize);
  drawTextGeneric(&luxStr[0], ST77XX_WHITE, data_indent, top_indent + 0*spacing, fontSize);

  switch(calculated){
    case 0: //iso is being calculated
      drawTextGeneric("ISO:", ST77XX_YELLOW, header_indent, top_indent + 1*spacing, fontSize);
      drawTextGeneric("APE: f", ST77XX_WHITE, header_indent, top_indent + 2*spacing, fontSize);
      drawTextGeneric("SHT:", ST77XX_WHITE, header_indent, top_indent + 3*spacing, fontSize);
      break;
    case 1: //aperture is being calculated
      drawTextGeneric("ISO:", ST77XX_WHITE, header_indent, top_indent + 1*spacing, fontSize);
      drawTextGeneric("APE: f", ST77XX_YELLOW, header_indent, top_indent + 2*spacing, fontSize);
      drawTextGeneric("SHT:", ST77XX_WHITE, header_indent, top_indent + 3*spacing, fontSize);
      break;
    case 2: //shutter is being calculated
      drawTextGeneric("ISO:", ST77XX_WHITE, header_indent, top_indent + 1*spacing, fontSize);
      drawTextGeneric("APE: f", ST77XX_WHITE, header_indent, top_indent + 2*spacing, fontSize);
      drawTextGeneric("SHT:", ST77XX_YELLOW, header_indent, top_indent + 3*spacing, fontSize);
      break;
  }

  //Display ISO and ISO value
  if(iso_pos >= iso_array_size - 1)
    iso_pos = iso_array_size - 1;
  if(iso_pos <= 0)
    iso_pos = 0;
  String iso_str = String(iso[iso_pos]);
  drawTextGeneric(&iso_str[0], ST77XX_WHITE, data_indent, top_indent + 1*spacing, fontSize);

  //Display Aperture and Aperture Value
  if(aperture_pos >= aperture_array_size - 1)
    aperture_pos = aperture_array_size - 1;
  if(aperture_pos <= 0)
    aperture_pos = 0;
  String aperture_str = String(aperture[aperture_pos]);
  
  drawTextGeneric(&aperture_str[0], ST77XX_WHITE, data_indent + 20, top_indent + 2*spacing, fontSize);

  //Display Shutter Speed and Shutter Speed Value
  if(shutter_pos >= shutter_array_size - 1)
    shutter_pos = shutter_array_size - 1;
  if(shutter_pos <= 0)
    shutter_pos = 0;
  String shutter_str = shutter_string[shutter_pos];

  drawTextGeneric(&shutter_str[0], ST77XX_WHITE, data_indent, top_indent + 3*spacing, fontSize);
  
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
