/*
  HardwareSerial.cpp - Hardware serial library for Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modified 23 November 2006 by David A. Mellis
  Modified 28 September 2010 by Mark Sproul
*/
#define HEX 16
#define DEC 10
#define BYTE 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "wiring.h"
#include "wiring_private.h"

// this next line disables the entire HardwareSerial.cpp, 
// this is so I can support Attiny series and any other chip without a uart
#if defined(UBRRH) || defined(UBRR0H) || defined(UBRR1H) || defined(UBRR2H) || defined(UBRR3H)

#include "HardwareSerial.h"

// Define constants and variables for buffering incoming serial data.  We're
// using a ring buffer (I think), in which rx_buffer_head is the index of the
// location to which to write the next incoming character and rx_buffer_tail
// is the index of the location from which to read.
#if (RAMEND < 1000)
  #define RX_BUFFER_SIZE 32
#else
  #define RX_BUFFER_SIZE 128
#endif

struct ring_buffer
{	
//  unsigned char buffer[RX_BUFFER_SIZE];
  unsigned int full_buffer[RX_BUFFER_SIZE];
  unsigned int address;
  unsigned int addressBits;
  int head;
  int tail;
  //volatile uint8_t *ucsra;
  //volatile uint8_t *ucsrb;
  uint8_t mpcm;
  uint8_t AddressFlag;
};

#if defined(UBRRH) || defined(UBRR0H)
  ring_buffer rx_buffer  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR1H)
  ring_buffer rx_buffer1  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR2H)
  ring_buffer rx_buffer2  =  { { 0 }, 0, 0 };
#endif
#if defined(UBRR3H)
  ring_buffer rx_buffer3  =  { { 0 }, 0, 0 };
#endif

inline void store_char(unsigned int data, ring_buffer *rx_buffer)
{
  //Serial.print("Interrupt Called: ");
  unsigned int mode = (data >> 8);
  unsigned char c = char(data & 0xFF);
  //Serial.println(c, HEX);
  unsigned int address = (rx_buffer->address & rx_buffer->addressBits);
  //volatile uint8_t *register_b = rx_buffer->ucsrb;
  //volatile uint8_t *register_a = rx_buffer->ucsra;
  uint8_t mpcm = rx_buffer->mpcm;
  uint8_t AddressFlag = rx_buffer->AddressFlag;
  uint8_t AddressBits = rx_buffer->addressBits;
  int i = (unsigned int)(rx_buffer->head + 1) % RX_BUFFER_SIZE;

  // if we should be storing the received character into the location
  // just before the tail (meaning that the head would advance to the
  // current location of the tail), we're about to overflow the buffer
  // and so we don't write the character or advance the head.
  if (i != rx_buffer->tail) 
  {
	if (mode && mpcm) //If mode bit is set and slave
	{
		//Serial.println("Checking Address");
		if ((int(c) & AddressBits) == address)
		{
			AddressFlag = 0;
		}
		else
		{
			AddressFlag = 1;
		}
		rx_buffer->AddressFlag = AddressFlag;
	}
	if (AddressFlag)
	{
		return;
	}
//	  rx_buffer->buffer[rx_buffer->head] = c;
	  rx_buffer->full_buffer[rx_buffer->head] = data;
	  rx_buffer->head = i;
  }
}

#if defined(USART_RX_vect)
  SIGNAL(USART_RX_vect)
  {
  #if defined(UDR0)
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSR0B >> 2) & 0x1)
	{
		c |= (((UCSR0B >> 1) & 0x1) << 8);
	}
    c  |=  UDR0;
  #elif defined(UDR)
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSRB >> 2) & 0x1)
	{
		c |= (((UCSRB >> 1) & 0x1) << 8);
	}
    c  |=  UDR;  //  atmega8535
  #else
    #error UDR not defined
  #endif
    store_char(c, &rx_buffer);
  }
#elif defined(SIG_USART0_RECV) && defined(UDR0)
  SIGNAL(SIG_USART0_RECV)
  {
	  unsigned int c = 0;
	  //Checks to see if using 9-bit
	  if ((UCSR0B >> 2) & 0x1)
	  {
		  c |= (((UCSR0B >> 1) & 0x1) << 8);
	  }
	  c  |=  UDR0;
	  store_char(c, &rx_buffer);
  }
#elif defined(SIG_UART0_RECV) && defined(UDR0)
  SIGNAL(SIG_UART0_RECV)
  {
	  unsigned int c = 0;
	  //Checks to see if using 9-bit
	  if ((UCSR0B >> 2) & 0x1)
	  {
		  c |= (((UCSR0B >> 1) & 0x1) << 8);
	  }
	  c  |=  UDR0;
	  store_char(c, &rx_buffer);
  }
//#elif defined(SIG_USART_RECV)
#elif defined(USART0_RX_vect)
  // fixed by Mark Sproul this is on the 644/644p
  //SIGNAL(SIG_USART_RECV)
  SIGNAL(USART0_RX_vect)
  {
  #if defined(UDR0)
	  unsigned int c = 0;
	  //Checks to see if using 9-bit
	  if ((UCSR0B >> 2) & 0x1)
	  {
		  c |= (((UCSR0B >> 1) & 0x1) << 8);
	  }
	  c  |=  UDR0;
  #elif defined(UDR)
	//  atmega8, atmega32
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSRB >> 2) & 0x1)
	{
		c |= (((UCSRB >> 1) & 0x1) << 8);
	}
	c  |=  UDR;
  #else
    #error UDR not defined
  #endif
    store_char(c, &rx_buffer);
  }
#elif defined(SIG_UART_RECV)
  // this is for atmega8
  SIGNAL(SIG_UART_RECV)
  {
  #if defined(UDR0)
      //  atmega645
	  unsigned int c = 0;
	  //Checks to see if using 9-bit
	  if ((UCSR0B >> 2) & 0x1)
	  {
		  c |= (((UCSR0B >> 1) & 0x1) << 8);
	  }
	  c  |=  UDR0;
  #elif defined(UDR)
	//  atmega8
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSR0B >> 2) & 0x1)
	{
	  c |= (((UCSR0B >> 1) & 0x1) << 8);
	}
	c  |=  UDR0;
  #endif
    store_char(c, &rx_buffer);
  }
#elif defined(USBCON)
  #warning No interrupt handler for usart 0
  #warning Serial(0) is on USB interface
#else
  #error No interrupt handler for usart 0
#endif

//#if defined(SIG_USART1_RECV)
#if defined(USART1_RX_vect)
  //SIGNAL(SIG_USART1_RECV)
  SIGNAL(USART1_RX_vect)
  {
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSR1B >> 2) & 0x1)
	{
		c |= (((UCSR1B >> 1) & 0x1) << 8);
	}
	c  |=  UDR1;
    store_char(c, &rx_buffer1);
  }
#elif defined(SIG_USART1_RECV)
  #error SIG_USART1_RECV
#endif

#if defined(USART2_RX_vect) && defined(UDR2)
  SIGNAL(USART2_RX_vect)
  {
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSR2B >> 2) & 0x1)
	{
	  c |= (((UCSR2B >> 1) & 0x1) << 8);
	}
	c  |=  UDR2;
    store_char(c, &rx_buffer2);
  }
#elif defined(SIG_USART2_RECV)
  #error SIG_USART2_RECV
#endif

#if defined(USART3_RX_vect) && defined(UDR3)
  SIGNAL(USART3_RX_vect)
  {
	unsigned int c = 0;
	//Checks to see if using 9-bit
	if ((UCSR3B >> 2) & 0x1)
	{
	  c |= (((UCSR3B >> 1) & 0x1) << 8);
	}
	c  |=  UDR3;
    store_char(c, &rx_buffer3);
  }
#elif defined(SIG_USART3_RECV)
  #error SIG_USART3_RECV
#endif



// Constructors ////////////////////////////////////////////////////////////////

HardwareSerial::HardwareSerial(ring_buffer *rx_buffer,
  volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
  volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
  volatile uint8_t *udr,
  uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t udre, uint8_t u2x)
{
  _rx_buffer = rx_buffer;
  //_rx_buffer->ucsra = ucsra;
  //_rx_buffer->ucsrb = ucsrb;
  _rx_buffer->mpcm = 0;
  _rx_buffer->address = 0x0;
  _rx_buffer->addressBits = 0x0;
  _rx_buffer->AddressFlag = 0;
  _ubrrh = ubrrh;
  _ubrrl = ubrrl;
  _ucsra = ucsra;
  _ucsrb = ucsrb;
  _udr = udr;
  _rxen = rxen;
  _txen = txen;
  _rxcie = rxcie;
  _udre = udre;
  _u2x = u2x;
}

// Public Methods //////////////////////////////////////////////////////////////

void HardwareSerial::begin(long baud)
{
  uint16_t baud_setting;
  bool use_u2x = true;

#if F_CPU == 16000000UL
  // hardcoded exception for compatibility with the bootloader shipped
  // with the Duemilanove and previous boards and the firmware on the 8U2
  // on the Uno and Mega 2560.
  if (baud == 57600) {
    use_u2x = false;
  }
#endif
  
  if (use_u2x) {
    *_ucsra = 1 << _u2x;
    baud_setting = (F_CPU / 4 / baud - 1) / 2;
  } else {
    *_ucsra = 0;
    baud_setting = (F_CPU / 8 / baud - 1) / 2;
  }

  // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
  *_ubrrh = baud_setting >> 8;
  *_ubrrl = baud_setting;

  sbi(*_ucsrb, _rxen);
  sbi(*_ucsrb, _txen);
  sbi(*_ucsrb, _rxcie);
}

void HardwareSerial::end()
{
  cbi(*_ucsrb, _rxen);
  cbi(*_ucsrb, _txen);
  cbi(*_ucsrb, _rxcie);  
}

int HardwareSerial::available(void)
{
  return (unsigned int)(RX_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % RX_BUFFER_SIZE;
}

int HardwareSerial::peek(void)
{
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    return _rx_buffer->full_buffer[_rx_buffer->tail];
  }
}

int HardwareSerial::read(void)
{
  // if the head isn't ahead of the tail, we don't have any characters
  if (_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    unsigned char c = _rx_buffer->full_buffer[_rx_buffer->tail];
    _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % RX_BUFFER_SIZE;
    return c;
  }
}

void HardwareSerial::flush()
{
  // don't reverse this or there may be problems if the RX interrupt
  // occurs after reading the value of rx_buffer_head but before writing
  // the value to rx_buffer_tail; the previous value of rx_buffer_head
  // may be written to rx_buffer_tail, making it appear as if the buffer
  // don't reverse this or there may be problems if the RX interrupt
  // occurs after reading the value of rx_buffer_head but before writing
  // the value to rx_buffer_tail; the previous value of rx_buffer_head
  // may be written to rx_buffer_tail, making it appear as if the buffer
  // were full, not empty.
  _rx_buffer->head = _rx_buffer->tail;
}

void HardwareSerial::write(uint8_t c)
{
  while (!((*_ucsra) & (1 << _udre)))
    ;

  *_udr = c;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

#if defined(UBRRH) && defined(UBRRL)
  HardwareSerial Serial(&rx_buffer, &UBRRH, &UBRRL, &UCSRA, &UCSRB, &UDR, RXEN, TXEN, RXCIE, UDRE, U2X);
#elif defined(UBRR0H) && defined(UBRR0L)
  HardwareSerial Serial(&rx_buffer, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UDR0, RXEN0, TXEN0, RXCIE0, UDRE0, U2X0);
#elif defined(USBCON)
  #warning no serial port defined  (port 0)
#else
  #error no serial port defined  (port 0)
#endif

#if defined(UBRR1H)
  HardwareSerial Serial1(&rx_buffer1, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UDR1, RXEN1, TXEN1, RXCIE1, UDRE1, U2X1);
#endif
#if defined(UBRR2H)
  HardwareSerial Serial2(&rx_buffer2, &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UDR2, RXEN2, TXEN2, RXCIE2, UDRE2, U2X2);
#endif
#if defined(UBRR3H)
  HardwareSerial Serial3(&rx_buffer3, &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UDR3, RXEN3, TXEN3, RXCIE3, UDRE3, U2X3);
#endif



MPCMHardwareSerial::MPCMHardwareSerial(ring_buffer *MPCM_rx_buffer,
					 volatile uint8_t *ubrrh, volatile uint8_t *ubrrl,
					 volatile uint8_t *ucsra, volatile uint8_t *ucsrb,
					 volatile uint8_t *udr,
					 uint8_t rxen, uint8_t txen, uint8_t rxcie, uint8_t mpcm, 
					 uint8_t ucsz2, uint8_t udre, uint8_t u2x)
{
	_MPCM_rx_buffer = MPCM_rx_buffer;
	//_MPCM_rx_buffer->ucsra = ucsra;
	//_MPCM_rx_buffer->ucsrb = ucsrb;
	_MPCM_rx_buffer->mpcm = 1;
	_MPCM_rx_buffer->address = 0xFF;
	_MPCM_rx_buffer->addressBits = 0xFF;
	_MPCM_rx_buffer->AddressFlag = 1;
	_ubrrh = ubrrh;
	_ubrrl = ubrrl;
	_ucsra = ucsra;
	_ucsrb = ucsrb;
	_udr = udr;
	_rxen = rxen;
	_txen = txen;
	_rxcie = rxcie;
	_mpcm = mpcm;
	_ucsz2 = ucsz2;
	_udre = udre;
	_u2x = u2x;
}

// Public Methods //////////////////////////////////////////////////////////////

void MPCMHardwareSerial::begin(long baud)
{
	this->begin(baud, 0);
}

void MPCMHardwareSerial::begin(long baud, uint8_t master)
{
	if (master)
	{
		_MPCM_rx_buffer->mpcm = 0;
		_MPCM_rx_buffer->address = 0x00;
		_MPCM_rx_buffer->addressBits = 0x00;
		_MPCM_rx_buffer->AddressFlag = 0;
	}
	else 
	{
		sbi(*_ucsra, _mpcm);
	}

	uint16_t baud_setting;
	bool use_u2x = true;
	
#if F_CPU == 16000000UL
	// hardcoded exception for compatibility with the bootloader shipped
	// with the Duemilanove and previous boards and the firmware on the 8U2
	// on the Uno and Mega 2560.
	if (baud == 57600) {
		use_u2x = false;
	}
#endif
	
	if (use_u2x) {
		*_ucsra = 1 << _u2x;
		baud_setting = (F_CPU / 4 / baud - 1) / 2;
	} else {
		*_ucsra = 0;
		baud_setting = (F_CPU / 8 / baud - 1) / 2;
	}
	
	// assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
	*_ubrrh = baud_setting >> 8;
	*_ubrrl = baud_setting;
	
	sbi(*_ucsrb, _rxen);
	sbi(*_ucsrb, _txen);
	sbi(*_ucsrb, _rxcie);
	sbi(*_ucsrb, _ucsz2); //Use 9 bits
}

void MPCMHardwareSerial::end()
{
	cbi(*_ucsra, _mpcm);
	cbi(*_ucsrb, _rxen);
	cbi(*_ucsrb, _txen);
	cbi(*_ucsrb, _rxcie);
	cbi(*_ucsrb, _ucsz2);
}

void MPCMHardwareSerial::setAddress(unsigned int address)
{
	unsigned int addressBits = 0xFF;
	_MPCM_rx_buffer->address = address;
	_MPCM_rx_buffer->addressBits = addressBits;
}

void MPCMHardwareSerial::setAddress(unsigned int address, unsigned int addressBits)
{
	_MPCM_rx_buffer->address = address;
	_MPCM_rx_buffer->addressBits = addressBits;
}	

int MPCMHardwareSerial::available(void)
{
	//Serial.println("Tail/Head");
	//Serial.println(_MPCM_rx_buffer->tail);
	//Serial.println(_MPCM_rx_buffer->head);
	return (unsigned int)(RX_BUFFER_SIZE + _MPCM_rx_buffer->head - _MPCM_rx_buffer->tail) % RX_BUFFER_SIZE;
}

int MPCMHardwareSerial::peek(void)
{
	if (_MPCM_rx_buffer->head == _MPCM_rx_buffer->tail) {
		return -1;
	} else {
		return _MPCM_rx_buffer->full_buffer[_MPCM_rx_buffer->tail];
	}
}

unsigned int MPCMHardwareSerial::read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if (_MPCM_rx_buffer->head == _MPCM_rx_buffer->tail) {
		return -1;
	} else {
		unsigned int c = _MPCM_rx_buffer->full_buffer[_MPCM_rx_buffer->tail];
		_MPCM_rx_buffer->tail = (unsigned int)(_MPCM_rx_buffer->tail + 1) % RX_BUFFER_SIZE;
		//Serial.print("C: ");
		//Serial.println(c, HEX);
		return c;
	}
}

void MPCMHardwareSerial::flush(void)
{
	// don't reverse this or there may be problems if the RX interrupt
	// occurs after reading the value of rx_buffer_head but before writing
	// the value to rx_buffer_tail; the previous value of rx_buffer_head
	// may be written to rx_buffer_tail, making it appear as if the buffer
	// don't reverse this or there may be problems if the RX interrupt
	// occurs after reading the value of rx_buffer_head but before writing
	// the value to rx_buffer_tail; the previous value of rx_buffer_head
	// may be written to rx_buffer_tail, making it appear as if the buffer
	// were full, not empty.
	_MPCM_rx_buffer->head = _MPCM_rx_buffer->tail;
}

//void MPCMHardwareSerial::write(uint8_t c)
//{
//	while (!((*_ucsra) & (1 << _udre)))
//		;//Wait for empty buffer
//	*_ucsrb &= 0xFE;//Sets 9th bit to 0
//	*_udr = (c & 0xFF); //Send Data bits
//}

void MPCMHardwareSerial::write(unsigned int c)
{
	while (!((*_ucsra) & (1 << _udre)));//Wait for empty buffer
//  *_ucsrb |= (c >> 8); //Set 9th bit
//	*_ucsrb |= 0x01; //Set Mode Bit
	*_ucsrb &= 0xFE; //Lower Data bit
	*_udr = (c & 0xFF); //Send Data bits
}
void MPCMHardwareSerial::writeMode(uint8_t c)
{
	while (!((*_ucsra) & (1 << _udre)));//Wait for empty buffer
//	*_ucsrb &= 0xFE;
//  *_ucsrb |= (c >> 8); //Set 9th bit
	*_ucsrb |= 0x01; //Set Mode Bit
	*_udr = (c & 0xFF); //Send Data bits
}
//
//void MPCMHardwareSerial::print(String string)
//{
//	int i;
//	for (i = 0; i < string.length(); i++)
//	{
//		if (i == string.length() - 1)
//		{
//			this->writeMode((unsigned int)string[i]);
//		}
//		this->write((uint8_t)string[i]);
//	}
//}
//
//void MPCMHardwareSerial::print(char c)
//{
//	this->writeMode((unsigned int)c);
//}
//
//void MPCMHardwareSerial::print(char c[])
//{
//	int i;
//	for (i = 0; c[i] != '\0'; i++)
//	{
//		if (c[i+1] == '\0')
//		{
//			this->writeMode((unsigned int)c[i]);
//		}
//		this->write((unsigned int)c[i]);
//	}
//}

//void MPCMHardwareSerial::print(char c[], uint8_t type)
//{
//	if (type == HEX)
//	{
//		

//void MPCMHardwareSerial::print(unsigned int i, uint8_t type = BYTE)
//{
//	if (type == BYTE)
//	{
//		this->writeMode(i);
//	}
//	else //if (type == DEC)
//	{
//		this->writeMode(i + 48);
//	}
//	//else if type
//}

//void MPCMHardwareSerial::print(unsigned int i[], uint8_t type = BYTE)
//{
//	int j;
//	if (type == BYTE)
//	{
//		for (j = 0; i[j] != 
	
// Preinstantiate Objects //////////////////////////////////////////////////////

#if defined(UBRRH) && defined(UBRRL)
	MPCMHardwareSerial MPCMSerial(&rx_buffer, &UBRRH, &UBRRL, &UCSRA, &UCSRB, &UDR, RXEN, TXEN, RXCIE, MPCM, UCSZ2, UDRE, U2X);
#elif defined(UBRR0H) && defined(UBRR0L)
	MPCMHardwareSerial MPCMSerial(&rx_buffer, &UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UDR0, RXEN0, TXEN0, RXCIE0, MPCM0, UCSZ02, UDRE0, U2X0);
#elif defined(USBCON)
	#warning no serial port defined  (port 0)
#else
	#error no serial port defined  (port 0)
#endif

#if defined(UBRR1H)
	MPCMHardwareSerial MPCMSerial1(&rx_buffer1, &UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UDR1, RXEN1, TXEN1, RXCIE1, MPCM1, UCSZ12, UDRE1, U2X1);
#endif
#if defined(UBRR2H)
	MPCMHardwareSerial MPCMSerial2(&rx_buffer2, &UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UDR2, RXEN2, TXEN2, RXCIE2, MPCM2, UCSZ22, UDRE2, U2X2);
#endif
#if defined(UBRR3H)
	MPCMHardwareSerial MPCMSerial3(&rx_buffer3, &UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UDR3, RXEN3, TXEN3, RXCIE3, MPCM3, UCSZ32, UDRE3, U2X3);
#endif

#endif // whole file

