# Dell/Alienware PSU Spoofer 
Allows user to use any lab power supply to power a Dell or Alienware notebook pc by supplying the required Dell PSU ID string. Dell and Alienware computers require the ID string to run normally. Performance is throttled if no ID string is provided for power protection purposes. The spoofer is configurable to send the ID string that matches the input lab supply's power rating via dipswitches. 

## Hardware Required
// Uses an Atmel ATtiny85 microcontroller and OneWireHub to emulate a Maxim DS2502, a 1Kb Add-Only Memory device 
// with a unique 48-bit serial number, programmable EPROM, and 1-Wire interface.
//
// KiCad Files
// dell_psu_spoofer.kicad_sch
// dell_psu_spoofer.kicad_pcb
// dell_psu_spoofer.csv
//
// Version 0.0   initial release for rev 0.0 pcb only
// Version 0.1   inverted logic to use internal pullup resistors of attiny85 - reliability
//               swapped pb5 and pb1 for c dipswitch pin to avoid rst conflict
//               requires rev 0.1 pcb or later
//
// TTFS Apps 2026
// ****************************************************************************************

## Libraries Required
|Library|Function|
|:-----:|:-----:|
|[OneWireHub.h](https://github.com/orgua/OneWireHub)|OneWireHub library| 
|DS2502.h|included with OneWireHub library| 


DIP Switch Settings
|A|B|C|Watts|
|:-----:|:--------:|:-------:|:-----:|
|0|0|0|100|
|0|0|1|150|
|0|1|0|200|
|0|1|1|250|
|1|0|0|300|
|1|0|1|350|
|1|1|0|400|
|1|1|1|500|


## Pin Names, Numbers, and Functions

|||||||||
|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
|ATtiny Pin|5|1|7|6|3|2|ATtiny85 hardware pin numbers (reference only)|
|Pin Name|PB0|PB5/RST|PB2|PB1|PB4|PB3|ATtiny85/Arduino pin names|
|Arduino Pin #|0|5|2|1|4|3|Arduino IDE pin numbers (s/w uses these)|
|Function|LED|RST|ID|A|B|C|Pin function|
|not used|OneWire|Power Select|Power Select|Power Select|
