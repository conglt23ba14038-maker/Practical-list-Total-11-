#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>
#define LCD_PORT PORTD
#define LCD_DDR  DDRD
#define LCD_RS   PD2
#define LCD_E    PD3
static void lcd_enable_pulse(void)
{
    LCD_PORT |= (1 << LCD_E);
    _delay_us(1);

    LCD_PORT &= ~(1 << LCD_E);
    _delay_us(50);
}

static void lcd_send_nibble(uint8_t value)
{    
    LCD_PORT = (LCD_PORT & 0x0F) | (value & 0xF0);
    lcd_enable_pulse();
}

static void lcd_command(uint8_t command)
{
    LCD_PORT &= ~(1 << LCD_RS);

    lcd_send_nibble(command);
    lcd_send_nibble(command << 4);

    if (command == 0x01 || command == 0x02)
        _delay_ms(2);
}

static void lcd_data(uint8_t data)
{
    LCD_PORT |= (1 << LCD_RS);

    lcd_send_nibble(data);
    lcd_send_nibble(data << 4);
}

static void lcd_init(void)
{
    LCD_DDR |=
        (1 << LCD_RS) |
        (1 << LCD_E)  |
        (1 << PD4)    |
        (1 << PD5)    |
        (1 << PD6)    |
        (1 << PD7);

    LCD_PORT &=
        ~((1 << LCD_RS) |
          (1 << LCD_E)  |
          (1 << PD4)    |
          (1 << PD5)    |
          (1 << PD6)    |
          (1 << PD7));

    _delay_ms(20);

    /* Khởi tạo LCD ở chế độ 4-bit */
    lcd_send_nibble(0x30);
    _delay_ms(5);

    lcd_send_nibble(0x30);
    _delay_us(150);

    lcd_send_nibble(0x30);
    lcd_send_nibble(0x20);

    lcd_command(0x28);  
    lcd_command(0x0C);  
    lcd_command(0x06);  
    lcd_command(0x01);  
}

static void lcd_set_cursor(uint8_t column, uint8_t row)
{
    uint8_t address;

    if (row == 0)
        address = 0x00 + column;
    else
        address = 0x40 + column;

    lcd_command(0x80 | address);
}

static void lcd_print(const char *text)
{
    while (*text != '\0')
    {
        lcd_data(*text);
        text++;
    }
}

static void lcd_print_uint32(uint32_t number)
{
    char buffer[11];
    uint8_t index = 0;

    if (number == 0)
    {
        lcd_data('0');
        return;
    }

    while (number > 0)
    {
        buffer[index++] = '0' + (number % 10);
        number /= 10;
    }

    while (index > 0)
        lcd_data(buffer[--index]);
}
#define TIMER1_FREQUENCY 2000000UL
volatile uint32_t timer1_overflows = 0;
volatile uint32_t previous_capture = 0;
volatile uint32_t captured_period = 0;
volatile uint8_t capture_ready = 0;
ISR(TIMER1_OVF_vect)
{
    timer1_overflows++;
}
ISR(TIMER1_CAPT_vect)
{
    uint16_t capture_value = ICR1;
    uint32_t overflow_value = timer1_overflows;
    uint32_t current_capture;
    if ((TIFR1 & (1 << TOV1)) && (capture_value < 0x8000))
        overflow_value++;

    current_capture =
        (overflow_value << 16) | capture_value;

    captured_period =
        current_capture - previous_capture;

    previous_capture = current_capture;
    capture_ready = 1;
}

static void timer1_input_capture_init(void)
{
    DDRB &= ~(1 << DDB0);
    PORTB &= ~(1 << PORTB0);
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    timer1_overflows = 0;
    previous_capture = 0;
    captured_period = 0;
    capture_ready = 0;
    TCCR1B =
        (1 << ICNC1) |
        (1 << ICES1) |
        (1 << CS11);
    TIFR1 =
        (1 << ICF1) |
        (1 << TOV1);
    TIMSK1 =
        (1 << ICIE1) |
        (1 << TOIE1);
}
int main(void)
{
    uint32_t period;
    uint32_t frequency;

    lcd_init();
    timer1_input_capture_init();

    lcd_set_cursor(0, 0);
    lcd_print("Input Capture");

    lcd_set_cursor(0, 1);
    lcd_print("Waiting...");

    sei();

    while (1)
    {
        if (capture_ready)
        {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                period = captured_period;
                capture_ready = 0;
            }

            if (period != 0)
            {
                frequency = TIMER1_FREQUENCY / period;

                lcd_set_cursor(0, 0);
                lcd_print("Frequency:      ");

                lcd_set_cursor(0, 1);
                lcd_print("                ");

                lcd_set_cursor(0, 1);
                lcd_print_uint32(frequency);
                lcd_print(" Hz");
            }
        }

        _delay_ms(100);
    }
}