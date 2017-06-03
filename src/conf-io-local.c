

// ATmega324 has full registers
#define HIGHEST_PORT_USED	I2C_ATMEGA_324

volatile uint8_t *gio_port_register[HIGHEST_PORT_USED +1] = {&PORTA, &PORTB, &PORTC, &PORTD};
volatile uint8_t *gio_pin_register[HIGHEST_PORT_USED +1] =  {&PINA, &PINB, &PINC, &PIND};
volatile uint8_t *gio_ddr_register[HIGHEST_PORT_USED +1] = {&DDRA, &DDRB, &DDRC, &DDRD};
    
uint8_t ddr_register_from_master[HIGHEST_PORT_USED +1];
uint8_t inputs_from_master[HIGHEST_PORT_USED +1];

