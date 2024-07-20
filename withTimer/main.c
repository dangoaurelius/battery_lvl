#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#ifdef DEBUG
#include "../lib/uart.h"
#endif

// Коефіцієнт для розрахунку остаточного значення напруги
#define COEFFICIENT             (85U)

#define SAMPLES_NUM             (4U)

#define LED_GRN_PIN             PB0
#define LED_YEL_PIN             PB1
#define LED_RED_PIN             PB2

// Граничний показник напруги в mV 8400 = 100% 6720 = 80%
#define BAT_GOOD_THRESHOLD      (8400U)
#define BAT_LOW_THRESHOLD       (6720U)
#define BAT_CRITICAL_THRESHOLD  (0)

#define CALL_ADC                (ADCSRA |= _BV(ADSC))

#define STATE_SLEEP             0
#define STATE_ADC               1
#define STATE_EXIT              2

static void handleCaptureVoltage(void);

// volatile змінні бо використовуються в інтерапторах
volatile uint16_t samplesSum   = 0;
volatile uint8_t  samplesCount = 0;
volatile uint8_t  state        = STATE_SLEEP;

int main(void)
{
  // Конфігуруємо режим сну в POWER DOWN енергозберігаючий режим
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Конфігуруємо пін кнопки
  GIMSK |= _BV(PCIE);   // Включаємо інтераптор PCINT
  PCMSK |= _BV(PCINT3); // Налаштовуємо інтераптор(кнопку) на PB3

  // Конфігуруємо АЦП
  ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  ADCSRA |= _BV(ADEN)  | _BV(ADIE);
  ADMUX   = _BV(MUX1)  | _BV(REFS0); // Працюємо з АЦП на піну BP4

  // Конфігуруємо піни для передачі данних
  #ifndef DEBUG
    DDRB = _BV(LED_GRN_PIN) |
           _BV(LED_YEL_PIN) |
           _BV(LED_RED_PIN);
  #endif

  sei(); // Вмикаємо глобальні інтераптори

  // Головний цикл програми
  while(state != STATE_EXIT) {
    if (state == STATE_SLEEP) {
      // Ресетимо піни перед режимом сну
      PORTB &= ~(_BV(LED_GRN_PIN) | _BV(LED_YEL_PIN) | _BV(LED_RED_PIN));
    
      // Включається режим енергозбереження
      sleep_enable();
      sleep_cpu();
    }
    
    if (state == STATE_ADC) {
      #ifdef DEBUG
      uart_puts("adc\n");
      #endif

      // Включається індикація напруги на ножці PB3
      handleCaptureVoltage();

      state = STATE_SLEEP;
    }
  }
}

void handleCaptureVoltage(void)
{
  CALL_ADC; // Вмикаємо флаг для отрипання значення АЦП

  uint8_t loopsCount = 0;
  while (loopsCount < 100) { // Сто рзів по 100 мілісекунд = приблизно 10 секунд
    // Якщо недостатньо семплів просто виходимо з функції
    if (samplesCount < SAMPLES_NUM) {
      continue;
    }

    uint16_t value;

    /* Розраховуємо напругу */
    value = samplesSum / (uint16_t) samplesCount; // середнє значення отриманих семплів

    // Відсоткове значення напруги де 1100mV(обрана опорна напруга) це 100%
    value = (uint16_t)(((uint32_t) value * 100UL) / 1024UL);

    // Розраховуємо остаточне значення напруги.
    value = value * COEFFICIENT;

    // Ресетимо кількість семплів і їх сумму
    samplesSum = 0;
    samplesCount = 0;

    loopsCount += 1;

    #ifdef DEBUG
    uart_putu(value);
    uart_puts("\n");
    #else
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

    CALL_ADC;
    _delay_ms(100);
  }
}

// Функції інтерапторів
// Інтераптор АЦП
ISR(ADC_vect)
{
  uint8_t high, low;

  if (state == 1 && samplesCount < SAMPLES_NUM) {
    low  = ADCL;
    high = ADCH;

    uint16_t val = (high << 8) | low;

    samplesSum += val;
    samplesCount += 1;

    CALL_ADC;
  }
}

// Інтераптор PCINT зпрацьовує на натискання кнопки,
// виводить з режиму енергозбереження
ISR(PCINT0_vect)
{
  // Змінюємо стейт тільки якщо в стані очікування команд
  if (state == STATE_SLEEP) {
    state = STATE_ADC;
    _delay_ms(400);
  }
}
