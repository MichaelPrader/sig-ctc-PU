
#ifndef BLOCK_DEFINES_H

#define N_PHYSICAL_BLOCK_INTERFACES	2

#include "block-defines-slave.h"
#endif

BlockRingBuffer BlockRXBuffer[N_PHYSICAL_BLOCK_INTERFACES];
BlockRingBuffer BlockTXBuffer[N_PHYSICAL_BLOCK_INTERFACES];

/*uint8_t IsOurAddress(uint8_t *addressField)
{
    if ((    (*(addressField +0) != OUR_ADDR0) |
            (*(addressField +1) != OUR_ADDR1) |
            (*(addressField +2) != OUR_ADDR2)) && // mismatch on the address
            
        (    (*(addressField +0) != 0xFF) |
            (*(addressField +1) != 0xFF) |
            (*(addressField +2) != 0xFF)) )     // mismatch on the broadcast
    {
        return 0;
    } else {
        return 1;
    }
} */


ISR(USART0_UDRE_vect)
{
	if (BlockRingBufferDepth(&(BlockTXBuffer[0])))
		UDR0 = BlockRingBufferPop(&(BlockTXBuffer[0]));
	else
		UCSR0B &= ~_BV(UDRIE0);
}

ISR(USART0_RX_vect)
{
	uint8_t status = UCSR0A, data = UDR0;

	// Framing errors and other crap.  Throw it out
	if (status & (_BV(FE0) | _BV(DOR0) | _BV(UPE0) ))
	{
		data = END;
	}
	
	//if (data == END) BlockRXBuffer[0].bufferState += 1;
	BlockRingBufferPush(&(BlockRXBuffer[0]), data);
}

#if (N_PHYSICAL_BLOCK_INTERFACES >= 2)
ISR(USART1_UDRE_vect)
{
	if (BlockRingBufferDepth(&(BlockTXBuffer[1])))
		UDR1 = BlockRingBufferPop(&(BlockTXBuffer[1]));
	else
		UCSR1B &= ~_BV(UDRIE1);
}


ISR(USART1_RX_vect)
{
	uint8_t status = UCSR1A, data = UDR1;

	// Framing errors and other crap.  Throw it out
	if (status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1) ))
	{
	    data = END;
	}
	
	//if (data == END) BlockRXBuffer[1].bufferState += 1;
	BlockRingBufferPush(&(BlockRXBuffer[1]), data);
}
#endif

void BlockUartInitialize(void)
{
	//UCSR0A
	UBRR0 = UBRRn_BAUD_DIVISOR(UART_BLOCK_BAUD);
//	UCSR0A = _BV(U2X0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
	
	
#if (N_PHYSICAL_BLOCK_INTERFACES >= 2)
		//UCSR1A
	UBRR1 = UBRRn_BAUD_DIVISOR(UART_BLOCK_BAUD);
//	UCSR1A = _BV(U2X1);
	UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
	UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
	
#endif

}

