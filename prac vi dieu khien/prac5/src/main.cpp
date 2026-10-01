#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
volatile uint8_t task_adc_flag = 0;
volatile uint8_t task_pwm_flag = 0;
volatile uint8_t task_sensor_flag = 0;
volatile uint8_t task_uart_flag = 0;
uint16_t adc_value = 0;
uint16_t sensor_value = 0;
uint8_t pwm_value = 0;
static void timer0_init(void)
{
    TCCR0A = (1 << WGM01);               
    TCCR0B = (1 << CS01) | (1 << CS00); 
    OCR0A = 249;

    TIMSK0 = (1 << OCIE0A);              
}
static void pwm_init(void)
{
    DDRB |= (1 << DDB1);                
    TCCR1A = (1 << COM1A1) | (1 << WGM10);
    TCCR1B = (1 << WGM12) |
             (1 << CS11) |
             (1 << CS10);               

    OCR1A = 0;                          
}
static void adc_init(void)
{
    ADMUX = (1 << REFS0); 

    ADCSRA = (1 << ADEN)  |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
}
static uint16_t adc_read(uint8_t channel)
{
    channel &= 0x07;
    ADMUX = (1 << REFS0) | channel;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADC;
}
static void uart_init(void)
{
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0A = 0;
    UCSR0B = (1 << TXEN0); 

    UCSR0C = (1 << UCSZ01) |
             (1 << UCSZ00); 
}

static void uart_send_char(char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

static void uart_send_string(const char *string)
{
    while (*string != '\0')
    {
        uart_send_char(*string);
        string++;
    }
}

static void uart_send_uint16(uint16_t number)
{
    char buffer[6];
    uint8_t index = 0;

    if (number == 0)
    {
        uart_send_char('0');
        return;
    }

    while (number > 0)
    {
        buffer[index] = (char)('0' + number % 10);
        number /= 10;
        index++;
    }

    while (index > 0)
    {
        index--;
        uart_send_char(buffer[index]);
    }
}

static void task_read_adc(void)
{
    adc_value = adc_read(0);
}
static void task_update_pwm(void)
{
    pwm_value = (uint8_t)(adc_value >> 2);
    OCR1A = pwm_value;
}
static void task_read_sensor(void)
{
    sensor_value = adc_read(1);
}
static void task_send_uart(void)
{
    uart_send_string("ADC: ");
    uart_send_uint16(adc_value);

    uart_send_string(", PWM: ");
    uart_send_uint16(pwm_value);

    uart_send_string(", Sensor: ");
    uart_send_uint16(sensor_value);

    uart_send_string("\r\n");
}
ISR(TIMER0_COMPA_vect)
{
    static uint16_t adc_counter = 0;
    static uint16_t pwm_counter = 0;
    static uint16_t sensor_counter = 0;
    static uint16_t uart_counter = 0;

    adc_counter++;
    pwm_counter++;
    sensor_counter++;
    uart_counter++;

    if (adc_counter >= 10)
    {
        adc_counter = 0;
        task_adc_flag = 1;
    }

    if (pwm_counter >= 20)
    {
        pwm_counter = 0;
        task_pwm_flag = 1;
    }

    if (sensor_counter >= 100)
    {
        sensor_counter = 0;
        task_sensor_flag = 1;
    }

    if (uart_counter >= 1000)
    {
        uart_counter = 0;
        task_uart_flag = 1;
    }
}

int main(void)
{
    adc_init();
    pwm_init();
    uart_init();
    timer0_init();

   
    sei();

    while (1)
    {
        if (task_adc_flag)
        {
            task_adc_flag = 0;
            task_read_adc();
        }

        if (task_pwm_flag)
        {
            task_pwm_flag = 0;
            task_update_pwm();
        }

        if (task_sensor_flag)
        {
            task_sensor_flag = 0;
            task_read_sensor();
        }

        if (task_uart_flag)
        {
            task_uart_flag = 0;
            task_send_uart();
        }
    }

    return 0;
}
