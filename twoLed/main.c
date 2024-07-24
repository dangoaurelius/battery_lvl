#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifdef DEBUG
#include "../lib/uart.h"
#endif

// Коефіцієнт для розрахунку остаточного значення напруги
#define COEFFICIENT             (85U)

#define SAMPLES_NUM             (32)

#define LED_FULL_PIN            PB0
#define LED_LOW_PIN             PB1

// Граничний показник напруги в mV 8400 = 100% 6700 = 75%
#define BAT_GOOD_THRESHOLD      (8000U) // 90%
#define BAT_LOW_THRESHOLD       (6700U) // 75%
#define BAT_CRITICAL_THRESHOLD  (0U)    // 0%

#define CALL_ADC                (ADCSRA |= _BV(ADSC))

volatile uint16_t samples_sum = 0;
volatile uint8_t  samples_cnt = 0;

int main(void)
{
  // Конфігуруємо піни для передачі данних
  #ifndef DEBUG
  DDRB = _BV(LED_FULL_PIN) | _BV(LED_LOW_PIN);
  #endif

  // Конфігуруємо АЦП
  ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA |= _BV(ADEN)  | _BV(ADIE);
  ADMUX   = _BV(MUX1)  | _BV(REFS0); // Працюємо з АЦП на піну BP4 (ліба UART використовує цей пін для отримання данних(RX) але нам це не треба тож спокійно юзаємо як АЦП)
  
  sei(); // Вмикаємо глобальні інтераптори
  
  CALL_ADC; // Вмикаємо флаг для отрипання значення АЦП

  while (1) {
    // Якщо недостатньо семплів викликаємо наступну ітерацію циклу
    if (samples_cnt < SAMPLES_NUM) {
      continue;
    }

    uint16_t value;

    /* Розраховуємо напругу */
    value = samples_sum / (uint16_t) samples_cnt; // середнє значення отриманих семплів

    // Відсоткове значення напруги де 1100mV(обрана опорна напруга) це 100%
    value = (uint16_t)(((uint32_t) value * 100UL) / 1024UL);

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
    PORTB &= ~(_BV(LED_FULL_PIN) | _BV(LED_LOW_PIN));

    if (value >= BAT_GOOD_THRESHOLD) {
      PORTB |= _BV(LED_FULL_PIN);
    } else if (value >= BAT_LOW_THRESHOLD) {
      PORTB |= _BV(LED_FULL_PIN);
      PORTB |= _BV(LED_LOW_PIN);
    } else if (value >= BAT_CRITICAL_THRESHOLD) {
      PORTB |= _BV(LED_LOW_PIN);
    }
    #endif

    CALL_ADC; // Трігеримо отримання наступного значення напруги

    _delay_ms(100);
  }
}

// АЦП інтераптор
ISR(ADC_vect)
{
  if (samples_cnt < SAMPLES_NUM) {
    uint8_t high, low;

    low  = ADCL;
    high = ADCH;

    samples_sum += (high << 8) | low;
    samples_cnt += 1;

    CALL_ADC;
  }
}