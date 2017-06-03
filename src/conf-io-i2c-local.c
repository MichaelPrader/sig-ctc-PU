

// define the port, and pins of I2C on this device
#define TWI_PORT    PORTC
#define TWI_PIN     PINC
#define TWI_DDR     DDRC

#define TWI_SDA     0x02        // PC1 on ATmega324
#define TWI_SCL     0x01        // PC0 on ATmega324
#define TWI_INT     0x04        // PC2 on ATmega324 // ok with schematics

#define TWI_ACTIVATE_INT    TWI_DDR |= TWI_INT
#define TWI_DEACTIVATE_INT  TWI_DDR &= ~TWI_INT