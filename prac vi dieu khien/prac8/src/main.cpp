#ifndef F_CPU
#define F_CPU 16000000UL 
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define SDA_PIN PC4
#define SCL_PIN PC5
#define I2C_PORT PORTC
#define I2C_DDR  DDRC
#define I2C_PIN  PINC

#define AHT20_ADDR 0x38


static inline void sda_high(void) { I2C_DDR &= ~(1 << SDA_PIN); I2C_PORT |= (1 << SDA_PIN); }
static inline void sda_low(void)  { I2C_DDR |= (1 << SDA_PIN);  I2C_PORT &= ~(1 << SDA_PIN); }
static inline void scl_high(void) {
    I2C_DDR &= ~(1 << SCL_PIN);
    I2C_PORT |= (1 << SCL_PIN);
   
    while (!(I2C_PIN & (1 << SCL_PIN)));
}
static inline void scl_low(void)  { I2C_DDR |= (1 << SCL_PIN); I2C_PORT &= ~(1 << SCL_PIN); }

void i2c_init(void) {
    sda_high();
    scl_high();
}

void i2c_start(void) {
    sda_high();
    scl_high();
    _delay_us(5);
    sda_low();
    _delay_us(5);
    scl_low();
}

void i2c_stop(void) {
    sda_low();
    _delay_us(5);
    scl_high();
    _delay_us(5);
    sda_high();
    _delay_us(5);
}


uint8_t i2c_write(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80) sda_high(); else sda_low();
        data <<= 1;
        _delay_us(3);
        scl_high();
        _delay_us(5);
        scl_low();
        _delay_us(3);
    }
    sda_high(); 
    _delay_us(3);
    scl_high();
    _delay_us(3);
    uint8_t ack = !(I2C_PIN & (1 << SDA_PIN)); 
    scl_low();
    return ack;
}

static uint8_t i2c_read(uint8_t send_ack) {
    uint8_t data = 0;
    sda_high();
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        scl_high();
        _delay_us(3);
        if (I2C_PIN & (1 << SDA_PIN)) data |= 1;
        scl_low();
        _delay_us(5);
    }
    if (send_ack) sda_low(); else sda_high();
    _delay_us(3);
    scl_high();
    _delay_us(5);
    scl_low();
    sda_high();
    return data;
}

uint8_t i2c_read_ack(void)  { return i2c_read(1); }
uint8_t i2c_read_nack(void) { return i2c_read(0); }


void uart_init(uint32_t baud) {
    uint16_t ubrr = (F_CPU / 16 / baud) - 1;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putc(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_print(const char *s) {
    while (*s) uart_putc(*s++);
}

void aht20_init_sensor(void) {
    _delay_ms(100); 

    i2c_start();
    i2c_write((AHT20_ADDR << 1) | 0);
    i2c_write(0xBE); 
    i2c_write(0x08);
    i2c_write(0x00);
    i2c_stop();

    _delay_ms(10);

    
    i2c_start();
    i2c_write((AHT20_ADDR << 1) | 1);
    uint8_t status = i2c_read_nack();
    i2c_stop();

    if (!(status & 0x08)) {
      
        i2c_start();
        i2c_write((AHT20_ADDR << 1) | 0);
        i2c_write(0xBE);
        i2c_write(0x08);
        i2c_write(0x00);
        i2c_stop();
        _delay_ms(10);
    }
}

uint8_t read_aht20(float *temp, float *hum) {
    // 1. Gửi lệnh kích hoạt đo
    i2c_start();
    if (!i2c_write((AHT20_ADDR << 1) | 0)) {
        i2c_stop();
        return 0; 
    }
    i2c_write(0xAC);
    i2c_write(0x33);
    i2c_write(0x00);
    i2c_stop();

    _delay_ms(75); 
    uint8_t status = 0;
    uint8_t timeout = 20; 
    while (timeout--) {
        i2c_start();
        i2c_write((AHT20_ADDR << 1) | 1);
        status = i2c_read_nack();
        i2c_stop();

        if (!(status & 0x80)) break; 
        _delay_ms(10);
    }

    if (status & 0x80) return 0; 


    i2c_start();
    i2c_write((AHT20_ADDR << 1) | 1);

    i2c_read_ack(); 
    uint8_t b1 = i2c_read_ack();
    uint8_t b2 = i2c_read_ack();
    uint8_t b3 = i2c_read_ack();
    uint8_t b4 = i2c_read_ack();
    uint8_t b5 = i2c_read_nack(); 
    i2c_stop();

    // 4. Ghép bit 20-bit
    uint32_t raw_hum  = (((uint32_t)b1 << 12) | ((uint32_t)b2 << 4) | (b3 >> 4));
    uint32_t raw_temp = ((((uint32_t)b3 & 0x0F) << 16) | ((uint32_t)b4 << 8) | b5);

    *hum  = ((float)raw_hum / 1048576.0f) * 100.0f;
    *temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

    return 1; 
}

int main(void) {
    char buffer[20];
    float temperature = 0.0f;
    float humidity = 0.0f;

    i2c_init();
    uart_init(9600);

    
    aht20_init_sensor();

    while (1) {
        if (read_aht20(&temperature, &humidity)) {
            uart_print("Nhiet do: ");
            dtostrf(temperature, 4, 1, buffer);
            uart_print(buffer);
uart_print(" C | Do am: ");
            dtostrf(humidity, 4, 1, buffer);
            uart_print(buffer);
            uart_print(" %\r\n");
        } else {
            uart_print("Loi giao tiep I2C!\r\n");
        }

        _delay_ms(2000);
    }
    return 0;
}