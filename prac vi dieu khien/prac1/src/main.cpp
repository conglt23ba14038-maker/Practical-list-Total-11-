#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL

#include <Arduino.h>

const int ledPins[] = {
    2, 3, 4, 5,
    6, 7, 8, 9
};

const int BUTTON = 10;
const int BUZZER = 11;

void setup()
{
   
    for (int i = 0; i < 8; i++)
    {
        pinMode(ledPins[i], OUTPUT);
    }
    pinMode(BUZZER, OUTPUT);
    pinMode(BUTTON, INPUT);
}

void loop()
{
    int buttonState = digitalRead(BUTTON);

    if (buttonState == HIGH)
    {
        for (int i = 0; i < 8; i++)
        {
            digitalWrite(ledPins[i], LOW);
        }

        digitalWrite(BUZZER, LOW);
    }
    else
    {
        
        for (int i = 0; i < 8; i++)
        {
            digitalWrite(ledPins[i], HIGH);
        }

        digitalWrite(BUZZER, HIGH);
    }
}
