#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#define PIR_PIN    PD2  
#define BUZZER_PIN PB5  


void uart_init(unsigned int baud) {
    unsigned int ubrr = (F_CPU / 16 / baud) - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);                   
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

// --- CHƯƠNG TRÌNH CHÍNH ---
int main(void) {
    DDRD &= ~(1 << PIR_PIN);    
    DDRB |= (1 << BUZZER_PIN);  

    uart_init(9600);
    _delay_ms(100);
    
    uart_print("PIR Motion Detection Active.\r\n");

    uint8_t last_state = 255; // Lưu trạng thái trước đó

    while (1) {
        
        uint8_t current_state = (PIND & (1 << PIR_PIN)) ? 1 : 0;

        
        if (current_state != last_state) {
            if (current_state == 1) {
                PORTB |= (1 << BUZZER_PIN);  
                uart_print("STATUS: Motion Detected! [Buzzer: ON]\r\n");
            } else {
                PORTB &= ~(1 << BUZZER_PIN);
                uart_print("STATUS: Clear - Motion Ended. [Buzzer: OFF]\r\n");
            }

            last_state = current_state;   
        }

        _delay_ms(50); 
    }

    return 0;
}