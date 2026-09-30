#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVO_PIN PB1 
void uart_init(unsigned int baud) {
    unsigned int ubrr = (F_CPU / 16 / baud) - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);                 
}

void uart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(const char *str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void uart_read_line(char *buffer, uint8_t max_len) {
    uint8_t idx = 0;
    while (1) {
        while (!(UCSR0A & (1 << RXC0)));
        char c = UDR0;

        // Process line return signals
        if (c == '\r' || c == '\n') {
            if (idx > 0) {
                buffer[idx] = '\0';
                return;
            }
        } else if (idx < max_len - 1) {
            buffer[idx++] = c;
        }
    }
}


void timer1_servo_init(void) {
    DDRB |= (1 << SERVO_PIN); 

    
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // Prescaler = 8

    ICR1 = 39999; 
    OCR1A = 3000; 
}

void set_servo_angle(uint8_t angle) {
   
    uint16_t pulse_ticks = 1000 + (((uint32_t)angle * 4000) / 180);
    OCR1A = pulse_ticks;
}


int main(void) {
    char rx_buf[16];
    char tx_buf[64];

    uart_init(9600);
    timer1_servo_init();

    uart_print("Servo Serial Controller Ready.\r\nCommand Format: Enter angle (0 - 180)\r\n");

    while (1) {
        uart_read_line(rx_buf, sizeof(rx_buf));
        int angle = atoi(rx_buf);

        
        if (angle < 0 || angle > 180) {
            uart_print("ERROR: Angle out of range! Allowed range: 0 - 180 degrees.\r\n");
        } else {
            set_servo_angle((uint8_t)angle);
            sprintf(tx_buf, "CONFIRMATION: Servo moved to %d degrees.\r\n", angle);
            uart_print(tx_buf);
        }
    }

    return 0;
}
