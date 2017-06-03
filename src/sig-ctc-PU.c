/*
 	
*/

/********************************* SYSTEM DEFINES AND INCLUDES ***************************/


#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <avr/wdt.h>



/***************************** COMMUNICATION-RELATED DEFINES AND INCLUDES ****************/

#include "conf-io-i2c-local.c"
#include "address.h"

#include "../../libs/i2c-com-defines/i2c-com-defines.h"
#include "TWI_slave.h"




/* SLIP special character codes
*/
#define END             0300    /* indicates end of packet 0xC0 */
#define ESC             0333    /* indicates byte stuffing 0xDB */
#define ESC_END         0334    /* ESC ESC_END means END data byte 0xDC */
#define ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte 0xDD */



// state indicator for TWI operation
TTWI_Operation TWI_Operation;

volatile uint8_t changed = 0;
#define INPUTS_CHANGED  0x01
#define BLOCK_CHANGED   0x02
#define QUARTER_SEC		0x80

#define TIMER_REQUEST_UPDATE    (8*4)    // eight seconds

/******************************* SPECIFIC INPUT AND OUTPUT *******************************/

#include "conf-io-local.c"

volatile uint8_t ticks = 0;


/********************************************************/

//#define N_BLOCKINTERFACES   2       // this type has always two interfaces configured
                                    // the pins CANNOT be used for standard I/O without tweaking
#include "block-slave.c"


/*******************************************************/

void init(void);


//#define STROBE  0x04
//#define STROBE_PORT PORTB
//#define STROBE_DDR  DDRB

#define ACCURATE_TIMER

#ifdef ACCURATE_TIMER

#define PRESCALER           64      // prescaler for timer is 64
#define FREQUENCY           100     // value is in Hz
#define TCNT1_RELOAD_VAL    (0xFFFF - F_CPU/(PRESCALER*FREQUENCY)) //0xF3CB//(0xFFFF - F_CPU/(PRESCALER*FREQUENCY))


// ******** Start 100 Hz Timer - Very Accurate Version

// Initialize a 100Hz timer for use in triggering events.
// If you need the timer resources back, you can remove this, but I find it
// rather handy in triggering things like periodic status transmissions.
// If you do remove it, be sure to yank the interrupt handler and ticks/secs as well
// and the call to this function in the main function

volatile uint8_t ticks;
volatile uint8_t quartersecs;

	

uint8_t restart = 0;

void initialize100HzTimer(void)
{
	// Set up timer 1 for 100Hz interrupts
	TCCR1A = 0;
	TCCR1B = _BV(CS11) | _BV(CS10);	// clk/64
	TCCR1C = 0;
	TIMSK1 |= _BV(TOIE1);
	ticks = 0;
}

ISR(TIMER1_OVF_vect)
{
	
	TCNT1 += TCNT1_RELOAD_VAL;
	
	++ticks;
	if (ticks >= 25)
	{
	   ticks = 0;
	   changed |= QUARTER_SEC;
	}
}
#endif

unsigned char TWI_Act_On_Failure_In_Last_Transmission ( unsigned char TWIerrorMsg )
{
                    // A failure has occurred, use TWIerrorMsg to determine the nature of the failure
                    // and take appropriate actions.
                    // Se header file for a list of possible failures messages.
    
//    TWI_Start_Transceiver_With_Data(messageBuf, 1);
    
  //  restart = 1;
    
/*    if (TWIerrorMsg == 0x00)
    {
        if (!(eeprom_is_ready())) eeprom_busy_wait();
        SlaveAddress = eeprom_read_byte((uint8_t *)EE_OWN_ADDRESS);
        TWI_Slave_Initialise( (SlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_GEN_BIT) );
    }
    */
    TWI_Start_Transceiver();
    
 return TWIerrorMsg;
}


int main(void)
{
    unsigned char messageBuf[TWI_BUFFER_SIZE];
    unsigned char i, j;
	unsigned char address;
	
	volatile uint8_t *port_pointer;
	
	uint8_t inputs[HIGHEST_PORT_USED +1];
	uint8_t old_inputs[HIGHEST_PORT_USED +1];
	
	uint8_t BlockActive[N_PHYSICAL_BLOCK_INTERFACES];
	
	

	
	
	//start initialization
	init();
	
	BlockUartInitialize();

	// read the address of this node from EEPROM location
	if (!(eeprom_is_ready())) eeprom_busy_wait();
    address = eeprom_read_byte((uint8_t *)EE_OWN_ADDRESS);
    
    // Initialise TWI module for slave operation. Include address and/or enable General Call.
    TWI_Slave_Initialise( (address << TWI_ADR_BITS) | (FALSE << TWI_GEN_BIT) );
	

	for (i = 0; i < N_PHYSICAL_BLOCK_INTERFACES; ++ i)
		BlockActive[i] = FALSE;


	initialize100HzTimer();


//STROBE_DDR |= STROBE;
//STROBE_PORT |= STROBE;
    
    
    
//    wdt_enable(WDTO_2S);
    
    
	sei();
	
	changed = 0;
			
	while(1)
	{
		

		
		/***************************************** INPUT  AND OUTPUT *************************/
		
        if (changed & QUARTER_SEC)
        {
            changed &= ~QUARTER_SEC;
			++quartersecs;

            // Events that happen repeatedly
    		if (quartersecs >= TIMER_REQUEST_UPDATE) {
                cli();
    		  changed |= INPUTS_CHANGED;
    		  quartersecs = 0;
    		  sei();
			  
			  
			
    	    }
    	}


		
		/********************* handle TWI access here **********************************/
		
	   // Check if the TWI Transceiver has completed an operation.
        if ( ! TWI_Transceiver_Busy() )
        {
        // Check if the last operation was successful
          if ( TWI_statusReg.lastTransOK )
          {
        // Check if the last operation was a reception
            if ( TWI_statusReg.RxDataInBuf )
            {
              TWI_Get_Data_From_Transceiver(messageBuf, TWI_Get_Data_Depth());      // with this length we don't need to save the length while receiving.
            
        // Check if the last operation was a reception as General Call
              if ( TWI_statusReg.genAddressCall )
              {
              // Put data received out to PORTB as an example.
                // nothing
              }
        // Ends up here if the last operation was a reception as Slave Address Match
              else
              {
                // master issued a command
                switch (messageBuf[I2C_CMD -1])
                {
                    case W_CMD_OUTPUTS_SET:
                        // master has sent bytes to put onto the output ports
                        // check which port pins have the DDR set and then set the PORT
                        for (i = 0; i <= I2C_ATMEGA_324; ++i)
                        {
                            port_pointer = gio_port_register[i];
                            for (j = 0; j < 8; ++j)
                            {
                                if (ddr_register_from_master[i] & (1<<j))
                                {
                                    // this is an output pin
                                    if (messageBuf[I2C_DATA-1+i] & (1<<j)) *port_pointer |= (1<<j); else *port_pointer &= ~(1<<j);
                                }
                            }
                        }
                        break;
                    
                    case W_CMD_ACTIVATE_OUTPUTS:
                        // master has sent the activation masks for the DDR
                        for (i = 0; i <= I2C_ATMEGA_324; ++i)
                        {
                            port_pointer = gio_ddr_register[i];
                            ddr_register_from_master[i] = 0;
                            
                            for (j = 0; j < 8; ++j)
                            {
                                // this is an output pin
                                if (messageBuf[I2C_DATA-1+i] & (1<<j))
                                {
                                    *port_pointer |= (1<<j);
                                    ddr_register_from_master[i] |= (1<<j);
                                } // By ignoring a bit that is not set, locally set outputs are not canceled.
                            }
                        }
                        break;
						
					case W_CMD_ACTIVATE_INPUTS:
                        // master has sent the activation masks for the DDR
                        for (i = 0; i <= I2C_ATMEGA_324; ++i)
                        {
                            inputs_from_master[i] = 0;
                            
                            for (j = 0; j < 8; ++j)
                            {
                                // this is an output pin
                                if (messageBuf[I2C_DATA-1+i] & (1<<j))
                                {
                                  
                                    inputs_from_master[i] |= (1<<j);
                                }
                            }
                        }
                        break;
                    
                    case W_CMD_BLOCK_PREPARE_DATA_LENGTH:
                        
                        TWI_Operation.ElementNumber = messageBuf[I2C_BLOCK_REMOTE_NUMBER -1];
                        if (TWI_Operation.ElementNumber < N_PHYSICAL_BLOCK_INTERFACES)
                        {
                            cli();
                            TWI_Operation.ReadLength = BlockRingBufferDepth(&BlockRXBuffer[TWI_Operation.ElementNumber]);
                            sei();
                        } else {
                            TWI_Operation.ReadLength = 0;
                            TWI_Operation.ElementNumber = 0;
                        }
						if (TWI_Operation.ReadLength > (I2C_MAX_MESSAGE_LENGTH)) TWI_Operation.ReadLength = I2C_MAX_MESSAGE_LENGTH;
                        
                        messageBuf[0] = TWI_Operation.ReadLength;
                        TWI_Operation.Operation = TWI_READ_BLOCK;
                        TWI_Operation.Suboperation = R_CMD_BLOCK_GET_DATA_LENGTH;
                        
                        TWI_Start_Transceiver_With_Data( messageBuf, R_CMD_BLOCK_GET_DATA_LENGTH_LEN-1);
                        // when the master reads 0, he will skip to the next block interface; there is no second check
                        // if the master still reads data, he will read input data, and probably classify it as garbage
                        
                        break;
                        
                        
                        
                    case W_CMD_INPUTS_PREPARE:
                        // prepare the data and start the transceiver
                        for (i = 0; i <= I2C_ATMEGA_324; ++i)
                        {
                          //  port_pointer = gio_pin_register[i];
                            messageBuf[i] = inputs[i];//*port_pointer;
                        }
                        TWI_Start_Transceiver_With_Data( messageBuf, I2C_ATMEGA_324 +1);

                        break;
                        
                    case W_CMD_BLOCK_WRITE_DATA:
                        TWI_Operation.ReadLength = messageBuf[I2C_LEN -1] -1;  // this includes I2C_CMD, I2C_LEN, I2C_BLOCK_REMOTE_NUMBER, data, but not the address byte
                        if (TWI_Operation.ReadLength > TWI_BUFFER_SIZE) TWI_Operation.ReadLength = TWI_BUFFER_SIZE;
                        
                        TWI_Operation.ElementNumber = messageBuf[I2C_BLOCK_REMOTE_NUMBER -1];
                        if (TWI_Operation.ElementNumber < N_PHYSICAL_BLOCK_INTERFACES)
                        {
                            for (i = I2C_BLOCK_REMOTE_NUMBER -1 +1; i < TWI_Operation.ReadLength; ++i)
                                BlockRingBufferPush(&(BlockTXBuffer[TWI_Operation.ElementNumber]), messageBuf[i]);
							BlockActive[TWI_Operation.ElementNumber] = TRUE;
                        }


                        
                        break;
                        
                     case W_CMD_OLD_INPUTS_PREP:
                        // prepare the data and start the transceiver
                        for (i = 0; i <= I2C_ATMEGA_324; ++i)
                        {
                            //port_pointer = gio_pin_register[i];
                            messageBuf[i] = old_inputs[i];
                        }
                        TWI_Start_Transceiver_With_Data( messageBuf, I2C_ATMEGA_324 +1);

                        //changed &= ~INPUTS_CHANGED;
                        break;
                    case W_CMD_CHANGED_PREP:
                        // prepare the data and start the transceiver
                       
                            messageBuf[0] = changed;
							messageBuf[1] = TWI_DDR;
                            TWI_Start_Transceiver_With_Data( messageBuf, 2);

                        //changed &= ~INPUTS_CHANGED;
                        break;
                      
  
                    default: break;
                }
              }
            }
        // Ends up here if the last operation was a transmission
            else
            {
                // as we don't have to expect a read of the block interface,
                // there is no need for a sequence control
                // we just leave this block free, the code will activate the transceiver at the block below
                
                // if operation before was a transmission, and we marked that as a TWI_READ_BLOCK,
                // then it was reading the Depth of the BlockRingBuffer
                if (TWI_Operation.Operation == TWI_READ_BLOCK)
                {
                    TWI_Operation.Operation = NO_OP;
                    if ((TWI_Operation.Suboperation == R_CMD_BLOCK_GET_DATA_LENGTH) &&
                        (TWI_Operation.ReadLength > 0)) // we sent a length to the master, and now must prepare to send it
                    {
                        for (i = 0; i < TWI_Operation.ReadLength; ++i)
                            messageBuf[i] = BlockRingBufferPop(&BlockRXBuffer[TWI_Operation.ElementNumber]);
                        
                        TWI_Start_Transceiver_With_Data(messageBuf,  TWI_Operation.ReadLength);
                    }
                }
                
            }
        // Check if the TWI Transceiver has already been started.
        // If not then restart it to prepare it for new receptions.
            if ( ! TWI_Transceiver_Busy() )
            {
              TWI_Start_Transceiver();
            }
          }
        // Ends up here if the last operation completed unsuccessfully
          else
          {
 //           TWI_Act_On_Failure_In_Last_Transmission( TWI_Get_State_Info() );
               TWI_Act_On_Failure_In_Last_Transmission( TWI_Get_State() );
          }
        }
        
        
		
		
		/* Test if something changed from the last time
		   around the loop - we need to send an update
		   packet if it did */
		for (i = 0; i <= I2C_ATMEGA_324; ++i)
		{
		    port_pointer = gio_pin_register[i];
		    inputs[i] = *port_pointer & inputs_from_master[i]; // read inputs and mask inputs only
		    
		    if (old_inputs[i] != inputs[i])
		    {
		        old_inputs[i] = inputs[i];
		        changed |= INPUTS_CHANGED;
		    }
		}
		
		for (i = 0; i < N_PHYSICAL_BLOCK_INTERFACES; ++i)
		{
		    if (BlockActive[i] && BlockRingBufferDepth(&BlockRXBuffer[i]))
		    {
		        changed |= BLOCK_CHANGED;
		    }
		}
		        
		
		if (changed & (BLOCK_CHANGED | INPUTS_CHANGED))
		{
		    // reset the update timer
            // ticks = 0;
		    TWI_ACTIVATE_INT;
		    changed &= ~(BLOCK_CHANGED | INPUTS_CHANGED);
		}
		
//if (TWI_DDR & TWI_INT) STROBE_PORT &= ~STROBE; else STROBE_PORT |= STROBE;
		
		if (BlockRingBufferDepth(&(BlockTXBuffer[0]))) ACTIVATE_BLOCK0_TX;
	#if (N_PHYSICAL_BLOCK_INTERFACES >= 2)
		if (BlockRingBufferDepth(&(BlockTXBuffer[1]))) ACTIVATE_BLOCK1_TX;
	#endif
	
	
	}
}



void init(void)
{
    
	// prescaler is 8, timer counts 1/2 microseconds
//	TCCR0 = _BV(CS01);

	
	TWI_DDR &= ~TWI_INT;
	TWI_PORT &= ~(TWI_SDA | TWI_SCL | TWI_INT); // disable pull-ups, set low
	
 //   GIMSK = 0x00;
	
	ACSR = _BV(ACD);
	ADCSRA = 0;
	
	
}
