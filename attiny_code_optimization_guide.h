/*
Sven's collected ATtiny85 C(++) code optimization tips for Arduino IDE

*************************************************************************************
DISCLAIMER:
  Please note, that these tips should not be considered "best practice" in general!
  Always try to create good code - but if everything else fails, give this a try ;)
 
  All product and company names are trademarks™ or registered® trademarks of their respective holders.
*************************************************************************************

* ALWAYS CHECK THE SIZE USING AN ATTINY85 CORE!
  Other controllers (like ATMEGA328P or ATMEGA2560) might react different to optimization
  or support other feature sets, resulting in smaller code sizes.

* Try different ATTINY cores. At the moment I'm using the ATTinyCore 
  from Spence Konde (v1.4.1) which has a small overhead.
  On my current project the ATTinyCore from Spence Konde delivers substantially 
  smaller code than the Damellis core.

  ATTinyCore by Spence Konde: http://drazzy.com/package_drazzy.com_index.json
  ATTINY core by Damellis: http://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json

* Don't use a bootloader, you don't need it and it will just cost flash memory.

* Enable link time optimization (LTO) if present, this will remove unused
  functions from the binary during linking, so you won't need to comment them out.
  
* If not required, disable 'millis()' and 'micros()' in the Spence Konde core - 
  this will save a large amount of flash (in my case more than 150 bytes!)

* Don't use the Arduino 'pinMode()' command, instead write directly to the control registers.
  This easily saves > 100 bytes of flash!
  For example the initialization of a TinyJoypad just requires these two lines of code:
  
  // configure A0, A3 and D1 as input
  DDRB &= ~( ( 1 << PB5) | ( 1 << PB3 ) | ( 1 << PB1 ) );
  // configure A2 as output
  DDRB |= ( 1 << PB4 );

* The AVR only supports bit shifting left/right by one bit at a time,
  so constructs with variable shifting like '1 << n' are quite expensive because 
  they require a loop.

* Don't initialize variables on declaration unless necessary, this saves at least
  2 bytes per variable.

* Check the use of global structures - using the local heap can be expensive :(
  
* Watch closely how your code changes affect the code size and/or the speed!
  Sometimes small changes result in much slower or much larger code.
  This can be either due to wrong assumptions or because the compiler decides
  to inline a function (faster, maybe larger) or not to inline it anymore (maybe smaller,
  but often much slower!). It is often difficult to determine which happened because 
  depending on the parameter list the non inlined code might be even larger.
  In these cases you might want to control inlining by using compiler attributes
  in the function prototypes, e.g.
  "void __attribute__ ((noinline)) foo();" or
  "void __attribute__ ((always_inline)) foo();"
  This can be painful to test, but may safe your day!
*/