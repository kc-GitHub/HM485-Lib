/**********************************************************/
/* Optiboot bootloader for Arduino                        */
/*                                                        */
/* http://optiboot.googlecode.com                         */
/*                                                        */
/* Arduino-maintained version : See README.TXT            */
/* http://code.google.com/p/arduino/                      */
/*                                                        */
/* Heavily optimised bootloader that is faster and        */
/* smaller than the Arduino standard bootloader           */
/*                                                        */
/*                                                        */
/* Assumptions:                                           */
/*   The code makes several assumptions that reduce the   */
/*   code size. They are all true after a hardware reset, */
/*   but may not be true if the bootloader is called by   */
/*   other means or on other hardware.                    */
/*     No interrupts can occur                            */
/*     UART and Timer 1 are set to their reset state      */
/*     SP points to RAMEND                                */
/*                                                        */
/*                                                        */
/**********************************************************/


#define __AVR_ATmega328P__
#define BAUD_RATE 19200L
#define BIG_BOOT
#define LED_START_FLASHES 5
#define TIMEOUT_MS 8000
//#define SOFT_UART


#define OPTIBOOT_MAJVER 4
#define OPTIBOOT_MINVER 4

#define MAKESTR(a) #a
#define MAKEVER(a, b) MAKESTR(a*256+b)

asm("  .section .version\n"
    "optiboot_version:  .word " MAKEVER(OPTIBOOT_MAJVER, OPTIBOOT_MINVER) "\n"
    "  .section .text\n");
	
#include <stdint.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include<util/delay.h>
 
// <avr/boot.h> uses sts instructions, but this version uses out instructions
// This saves cycles and program memory.
#include "boot.h"


// We don't use <avr/wdt.h> as those routines have interrupt overhead we don't need.

#include "pin_defs.h"
#include "stk500.h"





/****************************************************/
/*     Defines for HM485 protocol					*/
/****************************************************/

#define ENABLE_FRAME_START_SHORT 0

#define FRAME_START_SHORT 0xFE
#define FRAME_START_LONG 0xFD
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_DISCOVERY 4    // Discovery erkannt
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK
#define FRAME_ACKOK 16   // ACK OK
#define FRAME_CRCOK 32   // CRC OK

#define MAX_RX_FRAME_LENGTH 200


uint8_t frameComplete;
uint32_t targetAddress;
uint8_t frameDataLength;
static uint8_t frameData[200];
uint8_t startByte;
uint8_t frameControlByte;
uint32_t senderAddress;
uint32_t clientID;    
uint32_t programmerID;
uint8_t txFrameControlByte;
uint8_t txFrameDataLength;
uint8_t responseLength;

// Pin-Nummer fuer "TX-Enable"
uint8_t txEnablePin;
uint8_t frameStatus;
uint8_t rxDataIndex;
uint8_t txDataIndex;
//--------------------------------------------------------------------------------------




#ifndef LED_START_FLASHES
#define LED_START_FLASHES 0
#endif


/* Switch in soft UART for hard baud rates */
#if (F_CPU/BAUD_RATE) > 280 // > 57600 for 16MHz
#ifndef SOFT_UART
//#define SOFT_UART
#endif
#endif

/* Watchdog settings */
#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#ifndef __AVR_ATmega8__
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))
#endif

/* Function Prototypes */
/* The main function is in init9, which removes the interrupt vector table */
/* we don't need. It is also 'naked', which means the compiler does not    */
/* generate any entry or exit code itself. */
int main(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));
void putch(uint8_t);
uint8_t getch(void);
void rxNch(uint8_t);
void verifySpace();
void flash_led(uint8_t);
uint8_t getLen();
void watchdogReset();
void watchdogConfig(uint8_t x);
#ifdef SOFT_UART
void uartDelay() __attribute__ ((naked));
#endif
void appStart() __attribute__ ((naked));

/* Function Prototypes for HM485 communication */
void txch(uint8_t ch);
uint8_t rxch(void);
void receive();
void sendFrame();
void sendFrameByte(uint8_t sendByte);
unsigned int crc16Shift(uint8_t newByte , unsigned int oldCrc);




#if defined(__AVR_ATmega168__)
#define RAMSTART (0x100)
#define NRWWSTART (0x3800)
#elif defined(__AVR_ATmega328P__)
#define RAMSTART (0x200)		// old: 0x100
#define NRWWSTART (0x7000)
#elif defined (__AVR_ATmega644P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATtiny84__)
#define RAMSTART (0x100)
#define NRWWSTART (0x0000)
#elif defined(__AVR_ATmega1280__)
#define RAMSTART (0x200)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RAMSTART (0x100)
#define NRWWSTART (0x1800)
#endif

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff    ((uint8_t*)(RAMSTART))



/* main program starts here */
int main(void) {
  uint8_t ch;

  /*
   * Making these local and in registers prevents the need for initializing
   * them, and also saves space because code no longer stores to memory.
   * (initializing address keeps the compiler happy, but isn't really
   *  necessary, and uses 4 bytes of flash.)
   */

  // After the zero init loop, this is the first code to run.
  //
  // This code makes the following assumptions:
  //  No interrupts will execute
  //  SP points to RAMEND
  //  r1 contains zero
  //
  // If not, uncomment the following instructions:
  // cli();
  asm volatile ("clr __zero_reg__");
#ifdef __AVR_ATmega8__
  SP=RAMEND;  // This is done by hardware reset
#endif

  // Adaboot no-wait mod
  ch = MCUSR;
  MCUSR = 0;
  if (!(ch & _BV(EXTRF))) appStart();
  

#if LED_START_FLASHES > 0
  // Set up Timer 1 for timeout counter
  TCCR1B = _BV(CS12) | _BV(CS10); // div 1024
#endif



#ifndef SOFT_UART
#ifdef __AVR_ATmega8__
  UCSRA = _BV(U2X); //Double speed mode USART
  UCSRB = _BV(RXEN) | _BV(TXEN);  // enable Rx & Tx
  UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);  // config USART; 8N1
  UBRRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#else
  UCSR0A = _BV(U2X0); //Double speed mode USART0
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
//  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01) | _BV(UPM01);	// Parity Even
  UBRR0L = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#endif
#endif



  // Set up watchdog to trigger after 8s
  watchdogConfig(WATCHDOG_8S);

  /* Set LED pin as output */
  LED_DDR |= _BV(LED);

  /* Set Enable-Pin as output */
  ENBL_DDR |= _BV(ENBL);


#ifdef SOFT_UART
  /* Set TX pin as output */
  UART_DDR |= _BV(UART_TX_BIT);
#endif

#if LED_START_FLASHES > 0
  /* Flash onboard LED to signal entering of bootloader */
  flash_led(LED_START_FLASHES * 2);
#endif


  programmerID = 0x00000002;
  clientID = 0x0000ABCD;
  txDataIndex = 0;	


  register uint16_t address = 0;
  register uint8_t  length;


  /* Forever loop */
  for (;;) {
    /* get character from UART */
      
    ch = rxch();
	
    if(ch == STK_GET_PARAMETER) {
	  responseLength = 3;
	  uint8_t which = rxch();
      verifySpace();
      if (which == 0x82) {
		// Send optiboot version as "minor SW version"
		txch(OPTIBOOT_MINVER);
      }
	  else if (which == 0x81) {
		txch(OPTIBOOT_MAJVER);
      }
	  else {
	
		// * GET PARAMETER returns a generic 0x03 reply for
        // * other parameters - enough to keep Avrdude happy
	 	txch(0x03);
      }
    }
    else if(ch == STK_SET_DEVICE) {
	  responseLength = 2;
      // SET DEVICE is ignored
      rxNch(20);
    }
    else if(ch == STK_SET_DEVICE_EXT) {
	  responseLength = 2;
      // SET DEVICE EXT is ignored
      rxNch(5);
    }
    else if(ch == STK_LOAD_ADDRESS) {
	  responseLength = 2;
      // LOAD ADDRESS
      uint16_t newAddress;
      newAddress = rxch();
      newAddress = (newAddress & 0xff) | (rxch() << 8);
#ifdef RAMPZ
      // Transfer top bit to RAMPZ
      RAMPZ = (newAddress & 0x8000) ? 1 : 0;
#endif
      newAddress += newAddress; // Convert from word address to byte address
      address = newAddress;
      verifySpace();
    }
    else if(ch == STK_UNIVERSAL) {
	  responseLength = 3;
      // UNIVERSAL command is ignored
      rxNch(4);
      txch(0x00);
    }
    // Write memory, length is big endian and is in bytes 
    else if(ch == STK_PROG_PAGE) {
	  responseLength = 2;
      // PROGRAM PAGE - we support flash programming only, not EEPROM
      uint8_t *bufPtr;
      uint16_t addrPtr;

      rxch();	
      length = rxch();
      rxch();

  
      // If we are in RWW section, immediately start page erase
      if (address < NRWWSTART) __boot_page_erase_short((uint16_t)(void*)address);

      // While that is going on, read in page contents
      bufPtr = buff;
      do {
		*bufPtr++ = rxch();
      }
	  while (--length);
	  
	  	  
      // If we are in NRWW section, page erase has to be delayed until now.
      // Todo: Take RAMPZ into account
      if (address >= NRWWSTART) __boot_page_erase_short((uint16_t)(void*)address);

      // Read command terminator, start reply
      verifySpace();

      // If only a partial page is to be programmed, the erase might not be complete.
      // So check that here
      boot_spm_busy_wait();

      // Copy buffer into programming buffer
      bufPtr = buff;
      addrPtr = (uint16_t)(void*)address;
      ch = SPM_PAGESIZE / 2;
      do {
        uint16_t a;
        a = *bufPtr++;
        a |= (*bufPtr++) << 8;
        __boot_page_fill_short((uint16_t)(void*)addrPtr,a);
        addrPtr += 2;
      } while (--ch);

      // Write from programming buffer
      __boot_page_write_short((uint16_t)(void*)address);
      boot_spm_busy_wait();

#if defined(RWWSRE)
      // Reenable read access to flash
      boot_rww_enable();
#endif

    }
    // Read memory block mode, length is big endian.  
    else if(ch == STK_READ_PAGE) {
      // READ PAGE - we only read flash
      rxch();
      length = rxch();
      rxch();

  	  responseLength = length + 2;
	  
      verifySpace();

      do txch(pgm_read_byte_near(address++));
      while (--length);
    }
    // Get device signature bytes  
    else if(ch == STK_READ_SIGN) {
      // READ SIGN - return what Avrdude wants to hear
	  responseLength = 5;
      verifySpace();
      txch(SIGNATURE_0);
      txch(SIGNATURE_1);
      txch(SIGNATURE_2);
    }
    else if (ch == 'Q') {
      // Adaboot no-wait mod
	  responseLength = 2;
      watchdogConfig(WATCHDOG_16MS);
      verifySpace();

    }
    else {
      // This covers the response to commands like STK_ENTER_PROGMODE
	  responseLength = 2;
      verifySpace();
    }
    
    
    txch(STK_OK);
  }
}




#ifdef SOFT_UART
// AVR350 equation: #define UART_B_VALUE (((F_CPU/BAUD_RATE)-23)/6)
// Adding 3 to numerator simulates nearest rounding for more accurate baud rates
#define UART_B_VALUE (((F_CPU/BAUD_RATE)-20)/6)
#if UART_B_VALUE > 255
#error Baud rate too slow for soft UART
#endif

void uartDelay() {
  __asm__ __volatile__ (
    "ldi r25,%[count]\n"
    "1:dec r25\n"
    "brne 1b\n"
    "ret\n"
    ::[count] "M" (UART_B_VALUE)
  );
}
#endif


#if LED_START_FLASHES > 0
void flash_led(uint8_t count) {
  do {
    TCNT1 = -(F_CPU/(1024*16));
    TIFR1 = _BV(TOV1);
    while(!(TIFR1 & _BV(TOV1)));
#ifdef __AVR_ATmega8__
    LED_PORT ^= _BV(LED);
#else
    LED_PIN |= _BV(LED);
#endif
    watchdogReset();
  } while (--count);
}
#endif


void verifySpace() {
  if (rxch() != CRC_EOP) {
    watchdogConfig(WATCHDOG_16MS);    // shorten WD timeout
    while (1)			      // and busy-loop so that WD causes
      ;				      //  a reset and app start.
  }
  txch(STK_INSYNC);
}


void watchdogReset() {
  __asm__ __volatile__ (
    "wdr\n"
  );
}


void watchdogConfig(uint8_t x) {
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = x;
}


void appStart() {

  watchdogConfig(WATCHDOG_OFF);
  __asm__ __volatile__ (
#ifdef VIRTUAL_BOOT_PARTITION
    // Jump to WDT vector
    "ldi r30,4\n"
    "clr r31\n"
#else
    // Jump to RST vector	
	"clr r30\n"
    "clr r31\n"
#endif
    "ijmp\n"
  );
}







void txch(uint8_t ch) {

  frameData[txDataIndex] = ch;
  txDataIndex++;


  if (txDataIndex == responseLength) {
    txFrameDataLength = txDataIndex;
    sendFrame();
    txDataIndex = 0;
  }
}


void sendFrame() {

	  ENBL_PORT |= _BV(ENBL);
      //digitalWrite(txEnablePin, HIGH);
	  
      uint8_t tmpByte;

      unsigned int crc16checksum = 0xFFFF;
      txFrameControlByte = 0x78;

//    if(txTargetAddress != 0xFFFFFFFF){
        uint8_t txSeqNum = (frameControlByte >> 1) & 0x03;
        txFrameControlByte &= 0x9F;
        txFrameControlByte |= (txSeqNum << 5);
//    };


      putch(FRAME_START_LONG);  // send startbyte

      crc16checksum = crc16Shift(FRAME_START_LONG , crc16checksum);


      uint8_t i;
      uint32_t hmwAddress;
	  hmwAddress = programmerID;
	  
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = hmwAddress >> 24;
         sendFrameByte( tmpByte );
         crc16checksum = crc16Shift(tmpByte, crc16checksum);
         hmwAddress = hmwAddress << 8;
      };

      sendFrameByte(txFrameControlByte);                                      // send controll byte
      crc16checksum = crc16Shift(txFrameControlByte , crc16checksum);

      if(txFrameControlByte & 0b00001000){                                      // check if message has sender
    	  hmwAddress = clientID;
    	  for( i = 0; i < 4; i++){                                           // send sender address
    	    	 tmpByte = hmwAddress >> 24;
    	         sendFrameByte( tmpByte );
    	         crc16checksum = crc16Shift(tmpByte, crc16checksum);
    	         hmwAddress = hmwAddress << 8;
    	  }
      };
      tmpByte = txFrameDataLength + 2;                              // send data length
      sendFrameByte(tmpByte);
      crc16checksum = crc16Shift(tmpByte , crc16checksum);

    	for(i = 0; i < txFrameDataLength; i++){            // send data, falls was zu senden
            sendFrameByte(frameData[i]);                  // ----------------------------------------------------------
            crc16checksum = crc16Shift(frameData[i], crc16checksum);  // --------------------------------------------
    	}
      crc16checksum = crc16Shift(0 , crc16checksum);  // TODO: ???
      crc16checksum = crc16Shift(0 , crc16checksum);

      // CRC schicken
      sendFrameByte(crc16checksum / 0x100);   // TODO: High byte first? Ist das auch bei Adressen so?
      sendFrameByte(crc16checksum & 0xFF);

      frameStatus |= FRAME_SENTACKWAIT;
      frameStatus &= ~FRAME_ACKOK;

	  
	  _delay_ms(2);
	  //while (!(UCSR0A & _BV(TXC0)));
	  ENBL_PORT &= ~_BV(ENBL);
      //digitalWrite(txEnablePin, LOW);
	  
	  
} // sendFrame


void sendFrameByte(uint8_t sendByte) {


  // "Senden" einschalten
      //digitalWrite(txEnablePin, HIGH);
//      ENBL_PIN |= _BV(ENBL);
      
      if(sendByte == FRAME_START_LONG || sendByte == FRAME_START_SHORT || sendByte == ESCAPE_CHAR) {
         putch(ESCAPE_CHAR);
         sendByte &= 0x7F;
      };
      putch(sendByte);
  // "Senden" ausschalten
      //digitalWrite(txEnablePin, LOW);
//      ENBL_PIN &= ~_BV(ENBL);
      
};

  
void putch(uint8_t ch) {
  
#ifndef SOFT_UART
  while (!(UCSR0A & _BV(UDRE0)));
  UDR0 = ch;
#else
  __asm__ __volatile__ (
    "   com %[ch]\n" // ones complement, carry set
    "   sec\n"
    "1: brcc 2f\n"
    "   cbi %[uartPort],%[uartBit]\n"
    "   rjmp 3f\n"
    "2: sbi %[uartPort],%[uartBit]\n"
    "   nop\n"
    "3: rcall uartDelay\n"
    "   rcall uartDelay\n"
    "   lsr %[ch]\n"
    "   dec %[bitcnt]\n"
    "   brne 1b\n"
    :
    :
      [bitcnt] "d" (10),
      [ch] "r" (ch),
      [uartPort] "I" (_SFR_IO_ADDR(UART_PORT)),
      [uartBit] "I" (UART_TX_BIT)
    :
      "r25"
  );
#endif

}




void rxNch(uint8_t count) {
  do rxch(); while (--count);
  verifySpace();
}


uint8_t rxch(void) {
  uint8_t ch;


  if (rxDataIndex >= frameDataLength) {    // Keine Daten mehr im Puffer - neue Botschaft muss empfangen werden
    uint8_t dataReceived = 0;
    while(!dataReceived) {
      receive();
      if ((frameComplete)) {
		frameComplete = 0;
		if ((targetAddress == clientID) && (senderAddress == programmerID)) {
			rxDataIndex = 0;
			ch = frameData[rxDataIndex];
			dataReceived = 1;
			rxDataIndex++;
		}     
	  }	
    }   
    dataReceived = 0;
  }
  else {                         // Daten sind noch im Puffer
    ch = frameData[rxDataIndex];
    rxDataIndex++;
  }

  return ch;
  
}


void receive(){

  static uint8_t rxStartByte;
  static uint32_t rxTargetAddress;
  static uint8_t rxFrameControlByte;
  static uint32_t rxSenderAddress;
  static uint8_t rxFrameDataLength;       // L�nger der Daten + Checksum
  //static uint8_t rxFrameData[MAX_RX_FRAME_LENGTH];

  static uint8_t addressLength;
  static uint8_t addressLengthLong;
  static uint8_t framePointer;
  static uint8_t addressPointer;
  static unsigned int crc16checksum;
  
// TODO: Kann sich hier zu viel "anstauen", so dass das while vielleicht
//       nach ein paar Millisekunden unterbrochen werden sollte?


    uint8_t rxByte = getch();    // von Serial oder SoftSerial
	
    if(rxByte == ESCAPE_CHAR && !(frameStatus & FRAME_ESCAPE)){
// TODO: Wenn frameEscape gesetzt ist, dann sind das zwei Escapes hintereinander
//       Das ist eigentlich ein Fehler -> Fehlerbehandlung
     frameStatus |= FRAME_ESCAPE;
    }else{
      if(rxByte == FRAME_START_LONG || rxByte == FRAME_START_SHORT){  // Startzeichen empfangen
         rxStartByte = rxByte;
         frameStatus |= FRAME_START;
         frameStatus &= ~FRAME_ESCAPE;
         frameStatus &= ~FRAME_CRCOK;
         frameStatus &= ~FRAME_DISCOVERY;
         framePointer = 0;
         addressPointer = 0;
         rxSenderAddress = 0;
         rxTargetAddress = 0;
         crc16checksum = crc16Shift(rxByte , 0xFFFF);
         if(rxByte == FRAME_START_LONG){    // Startzeichen 0xFD
            addressLength = 4;
            addressLengthLong = 9;
         }
#if ENABLE_FRAME_START_SHORT          // Startzeichen 0xFE
         else{
            addressLength = 1;
            addressLengthLong = 2;
         }
#endif

       }else if(frameStatus & FRAME_START) {  // Startbyte wurde zuvor gesetzt und Frame wird nicht ignoriert
         if(frameStatus & FRAME_ESCAPE) {
            rxByte |= 0x80;
            frameStatus &= ~FRAME_ESCAPE;
         };
         crc16checksum = crc16Shift(rxByte , crc16checksum);
         if(addressPointer < addressLength){  // Adressbyte Zieladresse empfangen
            rxTargetAddress <<= 8;
            rxTargetAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer == addressLength){   // Controlbyte empfangen
            addressPointer++;
            rxFrameControlByte = rxByte;
            if ((rxFrameControlByte & 0b00000011) == 0b00000011){
              frameStatus |= FRAME_DISCOVERY;
            }
         }else if( (rxFrameControlByte & 0b00001000) && (addressPointer < addressLengthLong) && !(frameStatus & FRAME_DISCOVERY)) {
        	// Adressbyte Sender empfangen wenn CTRL_HAS_SENDER und FRAME_START_LONG
                // und kein Discovery angefordert ist (ControlByte = 0bXXXXXX11
        	rxSenderAddress <<= 8;
        	rxSenderAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer != 0xFF) { // Datenl�nge empfangen
            addressPointer = 0xFF;
            rxFrameDataLength = rxByte;
         }else{                   // Daten empfangen
            frameData[framePointer] = rxByte;   // Daten in Puffer speichern
//            rxFrameData[framePointer] = rxByte;   // Daten in Puffer speichern -------------------------------------------
            framePointer++;
            if(framePointer == rxFrameDataLength) {  // Daten komplett
// TODO: Fehlt da die Checksumme, oder ist rxFrameDataLength sowieso inklusive CRC?
//               crc16Register = crc16Shift(0 , crc16Register)
//               crc16Register = crc16Shift(0 , crc16Register)
               if(crc16checksum == 0) {    //
            	  frameStatus &= ~FRAME_START;
                  frameStatus |= FRAME_CRCOK;
                  // Framedaten f�r die sp�tere Verarbeitung speichern
                  // TODO: Braucht man das wirklich?
                  //       Moeglicherweise braucht man das nur, wenn mit Interrupts gearbeitet wird
                  startByte = rxStartByte;
                  targetAddress = rxTargetAddress;
                  frameControlByte = rxFrameControlByte;
                  senderAddress = rxSenderAddress;

                  frameDataLength = rxFrameDataLength - 2;
//                  for(i=0; i< frameDataLength; i++) {
//				    frameData[i] = rxFrameData[i];
//				  }
				  //memcpy(frameData, rxFrameData, frameDataLength);
                  framePointer = 0;
                  addressPointer = 0;
                  // es liegt eine neue Nachricht vor
                  frameComplete = 1;
				  
                  if(frameStatus & FRAME_DISCOVERY ) {
                    // Discovery Mode?
                    // Abfrage könnte auch in HMWHomebrew erfolgen (innerhalb der "hmwrs485.frameComplete-Abfrage")
                    // Aufruf müsste dann nicht aus dieser Funktion heraus erfolgen / public Variable notwendig
                    // processDiscovery();   // kein Discovery im Bootloader       
                  }

               }else{
                 // CRC-Fehler
				 // txch(0xFF);                 
                 // txch(crc16checksum >> 8);
                 // txch(crc16checksum);
                 // txch(0x20);
               }
            }
         }
      }
    }

  
} // receive


uint8_t getch(void) {
  uint8_t ch;

#ifdef LED_DATA_FLASH
#ifdef __AVR_ATmega8__
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif


#ifdef SOFT_UART
  __asm__ __volatile__ (
    "1: sbic  %[uartPin],%[uartBit]\n"  // Wait for start edge
    "   rjmp  1b\n"
    "   rcall uartDelay\n"          // Get to middle of start bit
    "2: rcall uartDelay\n"              // Wait 1 bit period
    "   rcall uartDelay\n"              // Wait 1 bit period
    "   clc\n"
    "   sbic  %[uartPin],%[uartBit]\n"
    "   sec\n"
    "   dec   %[bitCnt]\n"
    "   breq  3f\n"
    "   ror   %[ch]\n"
    "   rjmp  2b\n"
    "3:\n"
    :
      [ch] "=r" (ch)
    :
      [bitCnt] "d" (9),
      [uartPin] "I" (_SFR_IO_ADDR(UART_PIN)),
      [uartBit] "I" (UART_RX_BIT)
    :
      "r25"
);
#else
  while(!(UCSR0A & _BV(RXC0)))
    ;
  if (!(UCSR0A & _BV(FE0))) {
      /*
       * A Framing Error indicates (probably) that something is talking
       * to us at the wrong bit rate.  Assume that this is because it
       * expects to be talking to the application, and DON'T reset the
       * watchdog.  This should cause the bootloader to abort and run
       * the application "soon", if it keeps happening.  (Note that we
       * don't care that an invalid char is returned...)
       */
    watchdogReset();
  }
  
  ch = UDR0;
#endif




#ifdef LED_DATA_FLASH
#ifdef __AVR_ATmega8__
  LED_PORT ^= _BV(LED);
#else
  LED_PIN |= _BV(LED);
#endif
#endif

  return ch;
}    // getch


unsigned int crc16Shift(uint8_t newByte , unsigned int oldCrc) {
  unsigned int crc = oldCrc;
  uint8_t stat;
  uint8_t i;

  for (i = 0; i < 8; i++) {
    if (crc & 0x8000) {stat = 1;}
    else              {stat = 0;}
    crc = (crc << 1);
    if (newByte & 0x80) {
      crc = (crc | 1);
    }
    if (stat) {
      crc = (crc ^ CRC16_POLYNOM);
    }
    newByte = newByte << 1;
  }
  return crc;
}  // crc16Shift


