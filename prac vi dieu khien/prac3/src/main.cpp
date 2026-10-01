#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#define LED_PIN   PB0
#define SERVO_PIN PB1    
void timer1_servo_init(void)
{
    DDRB |= (1 << SERVO_PIN) | (1 << LED_PIN);
    PORTB &= ~(1 << LED_PIN);
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    ICR1 = 39999;
    OCR1A = 2000;
    TCCR1A = (1 << COM1A1) |
             (1 << WGM11);
    TCCR1B = (1 << WGM13) |
             (1 << WGM12) |
             (1 << CS11);
}
void servo_set_angle(uint16_t angle)
{
    if (angle > 180)
        angle = 180;
    OCR1A = 2000UL + ((uint32_t)angle * 2000UL / 180UL);
}
void led_on(void)
{
    PORTB |= (1 << LED_PIN);
}
void led_off(void)
{
    PORTB &= ~(1 << LED_PIN);
}
int main(void)
{
    timer1_servo_init();
    _delay_ms(500);
    while (1)
    {
        led_on();
        for (uint16_t angle = 0; angle <= 180; angle++)
        {
            servo_set_angle(angle);
            _delay_ms(10);
        }
        led_off();
        _delay_ms(500);
        led_on();
        for (int16_t angle = 180; angle >= 0; angle--)
        {
            servo_set_angle((uint16_t)angle);
            _delay_ms(10);
        }
        
        led_off();
        _delay_ms(500);
    }
}
