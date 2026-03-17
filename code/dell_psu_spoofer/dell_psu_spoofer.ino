// ****************************************************************************************
// Dell/Alienware PSU Spoofer 
//
// Allows any lab power supply to operate correctly with a Dell or Alienware notebook.
// The spoofer is configurable to send the ID string that matches the input lab supply's 
// power rating.

// Uses OneWireHub to emulate a Maxim DS2502, a 1Kb Add-Only Memory device 
// with a unique 48-bit serial number, programmable EPROM, and 1-Wire interface.
//
// KiCad Files
// dell_psu_spoofer.kicad_sch
// dell_psu_spoofer.kicad_pcb
//
// Version 0.0   initial release for rev 0.0 pcb only
// Version 0.1   inverted logic to use internal pullup resistors of attiny85 - reliability
//               swapped pb5 and pb1 for c dipswitch pin to avoid rst conflict
//               requires rev 0.1 pcb or later
//
// TTFS Apps 2026
// ****************************************************************************************

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

// onewirehub emulates numerous devices including the ds2502 used by dell/alienware psu's
#include <OneWireHub.h> // onewire hub library https://github.com/orgua/OneWireHub
#include "DS2502.h"     // included with onewirehub library

// DIP Switch Settings
// A   B   C	WATTS
// ----------------
// 0   0   0   100
// 0   0   1   150
// 0   1   0   200
// 0   1   1   250
// 1   0   0   300
// 1   0   1   350
// 1   1   0   400
// 1   1   1   500

// Pin Names, Numbers, and Functions
// ATtiny Pin #       5      1      7      6      3      2      // ATtiny85 hardware pin numbers (reference only)
// Pin Name          PB0  PB5/RST  PB2    PB1    PB4    PB3     // ATtiny85/Arduino pin names
// Arduino Pin #      0      5      2      1      4      3      // Arduino IDE pin numbers (s/w uses these)
// Function          LED    RST    ID      A      B      C      // Pin function
//                          NOT    ONE    PWR    PWR    PWR
//                          USED   WIRE   SEL    SEL    SEL

// declare pins
constexpr uint8_t aPin{1};    // PB5 first bit of pwr select
constexpr uint8_t bPin{4};    // PB4 second bit of pwr select
constexpr uint8_t cPin{3};    // PB3 third bit of pwr select
constexpr uint8_t idPin{2};   // PB2 dell id
constexpr uint8_t ledPin{0};  // PB0 led

String header = "DELL00AC";                                  // Dell ID header   DELL00AC
String volts = "195";                                        // Volts*10         VVV 3 digits in tenths of volts eg. 19.5 = 195
String watts = "100150200250300350400500";                   // Watts            WWW 3 digits
String amps = "051077103128154179205256";                    // Amps*10          III 3 digits in tenths of amps eg. 5.1 = 051
String serialNumber = "CN0123456789ABCDEA05";                // Serial number    CN0123456789ABCDEA05 random 23 character
String dellPSUId = "DELL00ACWWWVVVIIICN0123456789ABCDEA05";  // Dell ID          DELL00ACWWWVVVIIICN0123456789ABCDEA05 40 characters

char outBuffer[42] = "";                                     // dell id string + two crc bytes

auto hub = OneWireHub(idPin);                                // create onewire hub

// DS2502( family_code,  serial_1,  serial_2,  serial_3,  serial_4,  serial_5,  serial_6 );
auto dellPSURom = DS2502(0x09, 0x00, 0x00, 0x02, 0x25, 0x0D, 0x00); 

void setup() {

  // ******************
  // set up the spoofer
  // ******************

  // ************ debug code *************
  // verify 8mhz operation It should print: 8000000
  // If it prints 1000000, the attiny fuses aren't blown
  // and timing will fail.
  DEBUG_BEGIN(115200);   // serial baud rate
  DEBUG_PRINTLN(F_CPU);  // output attiny85 clock frequency
  // ************ debug code *************

  // set sense pins  for dip switch to digital input mode using internal pull up
  // note: rev 1 implements pull up logic
  pinMode(aPin, INPUT_PULLUP);  // first bit of pwr select
  pinMode(bPin, INPUT_PULLUP);  // second bit of pwr select
  pinMode(cPin, INPUT_PULLUP);  // third bit of pwr select

  // set led pin
  pinMode(ledPin, OUTPUT);  // led

  // set onewire connection
  pinMode(idPin, INPUT);  // input mode, don't use internal pull-up resistor

  // Fetch DIP Switch Settings new hardware inverts logic
  // calculate pointer to selected power based on aPin, bPin, and cPin vals
  uint8_t ptr = 4 * !digitalRead(aPin) + 2 * !digitalRead(bPin) + !digitalRead(cPin);
 
  // Build the identification string
  dellPSUId = header + watts.substring(ptr, ptr + 2) + volts + amps.substring(ptr, ptr + 2) + serialNumber;

  // Pad if needed
  while (dellPSUId.length() < 40) dellPSUId += ' ';

  // Calculate CRC
  uint16_t crc = crc16_arc_string(dellPSUId);

  uint8_t lowByte = crc & 0xFF;          // first CRC byte (little-endian)
  uint8_t highByte = (crc >> 8) & 0xFF;  // second CRC byte

  // Append raw crc bytes to the String (they may be non-printable!)
  // the crc bytes have to be in little-endian order
  // there is some debate as to whether all dell models use crc
  // now the string is 42 bytes long
  dellPSUId += (char)lowByte;           // second byte goes first
  dellPSUId += (char)highByte;          // first byte goes second

  // set up id string and stuff fake eeprom device
  dellPSUId.toCharArray(outBuffer, 42);
  dellPSURom.writeMemory((uint8_t *)outBuffer, 42, 0x00);

  // attach fake device to onewire hub
  hub.attach(dellPSURom);

}

void loop() {

  // ********************
  // start up the spoofer
  // ********************

  // signal life
  digitalWrite(ledPin, HIGH);  // LED on

  // onewirehub does all the hard work
  // Detects reset pulse from laptop
  // Answers presence
  // Handles Skip ROM (0xCC — Dell almost always uses this)
  // Responds to Read Memory (0xF0)
  // Provides bytes from the emulated memory 
  hub.poll();

}

uint16_t crc16_arc_string(const String &str) {

  // ******************************
  // routine tocalculate CRC-16/ARC
  // ******************************

  uint16_t crc = 0;                           // Start with crc = 0x0000

  // calculate crc
  // For each byte in the 40-byte string
  for (size_t i = 0; i < str.length(); i++) { 
    
    uint8_t byte = (uint8_t)str[i];           // fetch byte
    crc ^= (uint16_t)byte << 8;               // XOR the byte into the upper 8 bits
    
    // For each byte in the 40-byte str of crc (crc ^= byte << 8)
    for (uint8_t bit = 0; bit < 8; bit++) {   

    // If the current MSB (bit 15) is 1
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x8005;            // then shift left 1 bit, then XOR with 0x8005
      } else {
        crc <<= 1;                            // else just shift left 1 bit
      }
    }

  }

  // return crc
  return crc;
}

