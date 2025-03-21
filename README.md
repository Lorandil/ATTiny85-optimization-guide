# Lorandil's Collected ATtiny85 C(++) Code Optimization Tips for Arduino IDE (and gcc)

*************************************************************************************
DISCLAIMER:
  Please note, that these tips should not be considered "best practice" in general!<br>
  Always try to create good code - but if everything else fails, give these tips a try ;)
  
  I assembled these tips during my own projects, hopefully most observations are
  accurate. If you find any mistakes or have additional insights (or helpful) links, 
  please let me know :)
 
  All product and company names are trademarks™ or registered® trademarks of their respective holders.
*************************************************************************************

# General Optimization Tips
 
* ALWAYS CHECK THE SIZE USING AN ATTINY85 CORE!
  Other controllers (like ATMEGA328P or ATMEGA2560) might react different to optimization
  or support other feature sets, resulting in different code sizes.

* Try different ATtiny cores. At the moment I'm using the ATTinyCore 
  from Spence Konde (v1.5.2) which has a really small overhead.<br>
  On my current project the ATTinyCore delivers substantially smaller code than the Damellis core.

  ATTinyCore by Spence Konde:
  <br>https://github.com/SpenceKonde/ATTinyCore
  <br>http://drazzy.com/package_drazzy.com_index.json
  
  ATTINY core by Damellis: 
  <br>http://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
  
* Don't use a bootloader, you probably don't need it and it will just cost a lot of flash memory.

* Enable link time optimization (LTO) if present in your core, this will remove unused
  functions from the binary file during linking, so you won't need to comment out code.
   
* Don't initialize variables on declaration unless necessary, this saves at least
  2 bytes per variable.
  <br>Try block initialization with `memset()`.

* Check the use of global structures - using the local heap can be expensive :(

* Use a `default:` label on all `switch`-statements (if possible)!
  Even if all the sensible `case`s are coded, the compiler doesn't know that other values won't happen (but perhaps you do?!).
  Marking one of the existing cases as `default` enables the compiler to map all unhandled values to this
  case, which will save code:
  ```javascript
  uint8_t x;
  ...
  switch( x )
  {
  case 1:
  	...
	break;
  ...
  case 10:
  default: // adding 'default' to one 'case' saves flash memory
    	...
	break;
  }
  ```
  
   
* Avoid unnecessary `break` commands to leave a for-loop. In my tests a break cost > 20 bytes.
  <br>Sometimes not breaking only costs some more iterations and depending on the required speed it may 
  be acceptable to do some 'fruitless' iterations in favor of code size.
   
* Watch closely how your code changes affect the code size and/or the speed!
  Sometimes small changes result in much slower or much larger code.
  This can be either due to wrong assumptions or because the compiler decides
  to inline a function (faster, maybe larger) or not to inline it anymore (maybe smaller,
  but often much slower). It is often difficult to determine which happened because 
  depending on the parameter list the non inlined code might be even larger.
  In these cases you might want to control inlining by using compiler attributes
  in the function prototypes (This can be painful to test, but may safe your day!):
```javascript
  "void __attribute__ ((noinline)) foo();" or
  "void __attribute__ ((always_inline)) foo();"
```
  You might also want to try playing with the compiler's optimization strategy on a per function basis:
```javascript
__attribute__((optimize("O0"))) 
```

* Using floating point types comes at a cost, because the ATtinys don't have an FPU
  (so all operations have to be emulated in software).

* Avoid using powerful library functions like `sprintf()` or even `malloc()` because they require
  parts of the library to be included in the flash memory.
  This link shows some of the costs (in flash space and execution time) -
  *for ATtiny85 you have to refer to the Avr2 columns*.<br>
  http://www.nongnu.org/avr-libc/user-manual/benchmarks.html.
  
  
# Random Numbers
  
  If you occasionally need random numbers - don't use `rand()` and `srand()`.
  A free running timer without prescaler delivers fast changing numbers with a very small flash footprint:

  ```javascript
  // Initialize ATTiny85's timer 0 to run at full cpu clock (prescaler = 1).
  // The timer will count from its starting value (which does not require to be set for this use case)
  // to 255 and overflow to 0 and continue counting until the power is cut.
  TCCR0A = 0x00;          // normal mode
  TCCR0B = ( 1 << CS00 ); // prescaling with 1
  ```
  
  If you access the timer counter register after a user action, the insanely
  fast running timer returns an unpredictable number.
  The randomness of the value depends on the moment of the user action (which can be 
  expected to be random when working with a timer running at MHz speed).
  
  ```javascript
  // wait for some user action (e.g. button pressed)
  while( !isButtonPressed() );
  // access timer register to get an unpredictable number
  uint8_t nearRandomValue = TCNT0;
  ```
  
  Please note, that this method will only provide one(!) pseudo random number per user action.
  The next number will have a fix distance to the previous number (remember, it's just a counter after all)!
 
 
# ATtiny85 Specific Optimizations for Arduino IDE

  
* If not required, disable 'millis()' and 'micros()' in the ATtinyCore by Spence Konde - 
  this will save a large amount of flash (in my case more than 200 bytes!)
 
* Don't use the Arduino `pinMode()` command, instead write directly to the control registers.
  This easily saves > 100 bytes of flash!<br>
  For example the initialization of a TinyJoypad just requires these two lines of code:
  
```javascript
  // configure A0, A3 and D1 as input
  DDRB &= ~( ( 1 << PB5) | ( 1 << PB3 ) | ( 1 << PB1 ) );
  // configure A2 as output
  DDRB |= ( 1 << PB4 );
```

* The ATtiny85 only supports bit shifting left/right by one bit at a time,
  so constructs with variable shifting like `1 << n` are quite expensive because 
  they require a loop.
  Instead of
```javascript
  for ( uint8_t n = 0; n < 8; n++ ) {
	uint8_t bitValue = ( 1 << n ) 
	...
  }
```
  you might try
```javascript
  for ( uint8_t bitValue = 1; bitValue != 0; bitValue <<= 1 ) { ... }
```

* Sometimes the gcc creates different code sizes for equivalent lines (why? incomplete optimization for Avr2?)
```javascript
  uint8_t x, y;
  ...
  x = y / 2;  // is two to six (see below) bytes smaller
  x = y >> 1; //  than the equivalent bit shift
```

  Try this code to reproduce the problem:
```javascript
  extern uint8_t a;
  extern uint8_t b;
  extern uint8_t c[100];

  if ( a > b )
  {
    //b = a >> 1;	// 6 bytes larger! - why?
    b = a / 2;
    // c[] being some global uint8_t array (must be used in the program, otherwise gcc will remove the code!)
    *c = b;
  }
```
  
I took a closer look with Compiler Explorer (www.godbolt.org) and this was the result:
```javascript
  b = a >> 1;
```
compiles to an irritating 16 bit rotation (with the lsb being stored as a result)
```javascript
  ldi r25,0
  asr r25
  r24
```

whereas 
```javascript
  b = a / 2;
```
compiles as expected to
```javascript
lsr r24
```

Note: AVR gcc 14.2.0 creates the short version for both source code variants.
 
