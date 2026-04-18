// ****************************************************************************************
// Onewire Sniffer for Dell PSU's
//
// Displays Dell ID sring in hex and ASCII
//
// Tested on Arduino Mega 2560 with Elegoo 2.8" TFT Color Display Shield and Dell_PSU_Spoofer
//
// Version     Notes
//   0.0       initial release 
//   0.1       serial port only enabled for debug - reliability
//
// TTFS Apps 2026
// ****************************************************************************************

// Dell Power Adapter PCB Header Pins and Barrel Connector
// P1 inner barrel: +19.5 VDC
// P2 center pin: id 
// P3 outer shell: gnd

// Sniffer and Dell PSU Spoofer Pins
// Sniffer                 Spoofer
// ATmega Pin              ATTiny85 Pin
// 22  --------------->    <-------- PB2 onewire
// GND --------------->    <-------- GND
// not used ----------X    <-------- 19.5 VDC

// ****************debug macros*************************
#define DEBUG 0   // 1 = enable debug, 0 = disable debug

#if DEBUG
  #define DEBUG_BEGIN(baud)   Serial.begin(baud)
  #define DEBUG_PRINT(x)      Serial.print(x)
  #define DEBUG_PRINTLN(x)    Serial.println(x)
#else
  #define DEBUG_BEGIN(baud)   do {} while(0)
  #define DEBUG_PRINT(x)      do {} while(0)
  #define DEBUG_PRINTLN(x)    do {} while(0)
#endif
// ****************debug macros*************************

#include <MCUFRIEND_kbv.h>  // graphics library https://github.com/prenticedavid/MCUFRIEND_kbv
#include <Adafruit_GFX.h>   // adafruit-gfx-library https://github.com/adafruit/Adafruit-GFX-Library
#include <TouchScreen.h>    // touchscreen handler https://github.com/adafruit/Adafruit_TouchScreen
#include <OneWire.h>        // onewire library https://github.com/PaulStoffregen/OneWire/tree/master

// touchscreen pins (Elegoo defaults)
// XM, XP, YM, YP are the four wires of the resistive touchscreen layer
// XP ---- resistive layer ---- XM
// YP ---- resistive layer ---- YM
#define YP A3            
#define XM A2
#define YM 9
#define XP 8

// touchscreen calibration constants
#define TS_MINX 150
#define TS_MAXX 920
#define TS_MINY 120
#define TS_MAXY 940

// MCUFRIEND_kbv relies on Adafruit_GFX, which does not define named 
// colors by default — only raw 16-bit RGB565 values.
// this fixes the compile error
#define BLACK   0x0000   
#define BLUE    0x001F  
#define RED     0xF800   
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// set up touch screen
MCUFRIEND_kbv tft;

// pin numbers connected to the touch screen's X+ (XP), Y+ (YP), X- (XM), and Y- (YM)
// 300 ohms resistance across the X-plate of the touchscreen
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); 

// the sniffer is the master 
#define ONE_WIRE_BUS 22  // this is the onewire pin

// create onewire object
OneWire ds(ONE_WIRE_BUS);  

// ds2502 memory organization - see datasheet for details 
#define TOTAL_BYTES 128                        // 1024 bits
#define PAGE_SIZE 16                           // 16 bytes per page
#define TOTAL_PAGES (TOTAL_BYTES / PAGE_SIZE)  // 8 pages

// ds2502 rom id and memory configuration
uint8_t romID[8];  // [0]  Family code, [1–6]  Unique serial number, [7]  CRC8 of first 7 bytes
uint8_t romMemory[TOTAL_BYTES];
uint8_t currentPage = 0;

void setup() {
  
  DEBUG_BEGIN(115200);  // added debug in version 0.1

  // set up tft display parameters
  uint16_t id = tft.readID();
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setTextColor(GREEN, BLACK);
  tft.setTextSize(2);

  // say hello
  tft.setCursor(10, 10);
  tft.println("Dell/Alien PSU Sniffer");

  // say goodbye if onewire can't be found or read
  if (!findDS2502()) return;
  if (!readROM()) return;

  // display onewire data and scroll buttons
  drawPage();  

}

void loop() {

  // handle scroll buttons
  handleTouch();  

}

// ---------------------------------------------------------------------------------------
// --------------------------------------- OneWire --------------------------------------- 
// ---------------------------------------------------------------------------------------

bool findDS2502() {

  // *************
  // error handler
  // *************

  // is there anyone out there?
  if (!ds.search(romID)) {
    errorMsg("No OneWire device");
    return false;
  }

  // who are you?
  if (romID[0] != 0x09) { 
    errorMsg("Not DS2502");
    return false;
  }

  // i didn't catch that
  if (OneWire::crc8(romID, 7) != romID[7]) {
    errorMsg("ROM CRC fail");
    return false;
  }

  // success
  infoMsg("ROM OK");
  tft.setTextColor(GREEN); // why back to green

  return true;

}

void errorMsg(const char *msg) {

  // **********************
  // routine to print error 
  // **********************

  tft.setTextColor(RED);  // paint it red
  tft.println(msg);       // print it's dead
  while (1);              // just wait

}
void infoMsg(const char *msg) {

  // **********************
  // routine to print error 
  // **********************

  tft.setTextColor(GREEN);  // green is good
  tft.println(msg);       // print info text

}
bool readROM() {

  // ***********************
  // routine to read the rom
  // ***********************

  // This generates a 1-Wire reset pulse:
  // Master pulls the bus low for ~480 µs
  // Releases the line
  // Waits for a presence pulse from a device
  ds.reset();

  // set up read
  ds.select(romID); // select rom
  ds.write(0xF0);   // READ MEMORY command
  ds.write(0x00);   // TA1 (Target Address low byte)
  ds.write(0x00);   // TA2 (Target Address high byte)

  // get rom data
  for (int i = 0; i < TOTAL_BYTES; i++) {
    romMemory[i] = ds.read();
  }

  return true;

}

// ---------------------------------------------------------------------------------------
// --------------------------------------- Display --------------------------------------- 
// ---------------------------------------------------------------------------------------

void drawPage() {

  // ****************************
  // routine to draw output pages
  // ****************************

  tft.fillRect(0, 40, tft.width(), tft.height() - 40, BLACK);

  tft.setTextSize(1);
  tft.setCursor(5, 45);

  uint8_t base = currentPage * PAGE_SIZE;

  // Print address
  if (base < 16) tft.print("0");
  tft.print(base, HEX);
  tft.print(": ");

  // Print hex values
  for (int i = 0; i < PAGE_SIZE; i++) {
    uint8_t val = romMemory[base + i];

    if (val < 16) tft.print("0");  // pad value with leading 0 if less than 16 
    
    tft.print(val, HEX);           // print two hex chars
    tft.print(" ");                // spacer

  }

  // Spacer
  tft.print(" | ");

  // Print ASCII translation
  // DELL00ACWWWVVVIIICN0123456789ABCDEA05
  for (int i = 0; i < PAGE_SIZE; i++) {
    uint8_t c = romMemory[base + i];

    if (c >= 32 && c <= 126) {
      tft.write(c);   // printable ASCII
    } else {
      tft.print("."); // non-printable
    }

  }
  
  // Draw footer
  drawFooter();

}

void drawFooter() {

  // **********************
  // footer display routine
  // **********************

  // scroll buttons
  tft.setCursor(5, tft.height() - 15);
  tft.print("< Prev");
  tft.setCursor(tft.width() - 50, tft.height() - 15);
  tft.print("Next >");

  // page numbering
  tft.setTextSize(1);
  tft.setCursor(10, tft.height() - 15);
  tft.print("Page ");
  tft.print(currentPage + 1);
  tft.print("/");
  tft.print(TOTAL_PAGES);

}

// -------------------------------------------------------------------------------------
// --------------------------------------- Touch --------------------------------------- 
// -------------------------------------------------------------------------------------

void handleTouch() {

  // ***************************
  // touchscreen buttons handler
  // ***************************

  // Measure X position
  // Drive Y axis with voltage
  // Read voltage on X axis
  // YP → HIGH
  // YM → LOW
  // XP → analog read
  // XM → floating

  // Measure Y position
  // Drive X axis
  // Read Y axis
  // XP → HIGH
  // XM → LOW
  // YP → analog read
  // YM → floating

  // get touch point
  TSPoint touchPoint = ts.getPoint();

  // restore pin function - a library quirk or bad coding?
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // return if screen is not being touched
  if (touchPoint.z < 200 || touchPoint.z > 1000) return;

  // fetch coordinates
  int x = map(touchPoint.x, TS_MINX, TS_MAXX, 0, tft.width());
  int y = map(touchPoint.y, TS_MINY, TS_MAXY, 0, tft.height());

  // Bottom left corner = previous
  if (x < 60 && y > tft.height() - 40) {

    if (currentPage > 0) {
      currentPage--;
      drawPage();
      delay(250);
    }

  }

  // Bottom right corner = next
  if (x > tft.width() - 60 && y > tft.height() - 40) {
    
    if (currentPage < TOTAL_PAGES - 1) {
      currentPage++;
      drawPage();
      delay(250);
    }

  }

}


