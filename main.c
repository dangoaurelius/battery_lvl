#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifdef DEBUG
#include "uart.h"
#endif

// Коефіцієнт для розрахунку остаточного значення напруги
#define COEFFICIENT             (85U)

#define SAMPLES_NUM             (4)

#define LED_GRN_PIN             PB0
#define LED_YEL_PIN             PB1
#define LED_RED_PIN             PB2

// Граничний показник напруги в mV 8400 = 100% 6720 = 80%
#define BAT_GOOD_THRESHOLD      (8400U)
#define BAT_LOW_THRESHOLD       (6720U)
#define BAT_CRITICAL_THRESHOLD  (0)

#define CALL_ADC                (ADCSRA |= _BV(ADSC))

static void process(void);

volatile uint16_t samples_sum = 0;
volatile uint8_t  samples_cnt = 0;

ISR(ADC_vect)
{
    uint8_t high, low;

    if (samples_cnt < SAMPLES_NUM) {
        low  = ADCL;
        high = ADCH;

        uint16_t val = (high << 8) | low;

        samples_sum += val;
        samples_cnt++;

        CALL_ADC;
    }
}

int main(void)
{
  // Конфігуруємо піни для передачі данних
  #ifndef DEBUG
    DDRB = _BV(LED_GRN_PIN) |
           _BV(LED_YEL_PIN) |
           _BV(LED_RED_PIN);
  #endif

  // Конфігуруємо АЦП
  ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA |= _BV(ADEN)  | _BV(ADIE);
  ADMUX   = _BV(MUX1)  | _BV(REFS0); // Працюємо з АЦП на піну BP4 (ліба UART використовує цей пін для отримання данних(RX) але нам це не треба тож спокійно юзаємо як АЦП)
  
  sei(); // Вмикаємо глобальні інтераптори

  CALL_ADC; // Вмикаємо флаг для отрипання значення АЦП

  while (1) {
    process();
    _delay_ms(100);
  }
}

void process(void)
{
  uint16_t value;

  // Якщо недостатньо семплів просто виходимо з функції
  if (samples_cnt < SAMPLES_NUM) {
    return;
  }

  /* Розраховуємо напругу */
  value = samples_sum / (uint16_t) samples_cnt; // середнє значення отриманих семплів
  #ifdef DEBUG
  uart_putu(value);
  uart_puts(" \0");
  #endif

  // Відсоткове значення напруги де 1100mV(обрана опорна напруга) це 100%
  value = (uint16_t)(((uint32_t) value * 100UL) / 1024UL);
  #ifdef DEBUG
  uart_putu(value);
  uart_puts(" \0");
  #endif

  // Розраховуємо остаточне значення напруги.
  value = value * COEFFICIENT;
  #ifdef DEBUG
  uart_putu(value);
  uart_puts("\n\0");
  _delay_ms(1000);
  #endif

  // Ресетимо кількість семплів і їх сумму
  samples_sum = 0;
  samples_cnt = 0;

  #ifndef DEBUG
  // Ресетимо піни перед встановленням наступного значення
  PORTB &= ~(_BV(LED_GRN_PIN) | _BV(LED_YEL_PIN) | _BV(LED_RED_PIN));


  if (value >= BAT_GOOD_THRESHOLD) {
    PORTB |= _BV(LED_GRN_PIN);
  } else if (value >= BAT_LOW_THRESHOLD) {
    PORTB |= _BV(LED_YEL_PIN);
  } else if (value >= BAT_CRITICAL_THRESHOLD) {
    PORTB |= _BV(LED_RED_PIN);
  }
  // Тут можна розширити функціонад нарпиклад зробити мигаючим
  // червоний коли заряду менше 30№ а якщо більше 30 то просто червоний
  #endif

  CALL_ADC; // Трігеримо отримання наступного значення напруги
}