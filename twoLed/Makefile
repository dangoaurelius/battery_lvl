MCU=attiny13a
FUSE_L=0x6A
FUSE_H=0xFF
# Частота мікроконтреоллеру. Налаштовується фьюзами. В данному випадку
# частота 9.6 магагерц але ввімкнений дівайдер на 8
# того частоста 9600000 / 8 = 1200000
F_CPU=1200000

# Дебаг режим - замість виводу в вигляді ргб світодіодів
# передає значення вольтажу через UART.
# DEBUG=1

CC=avr-gcc
LD=avr-ld
OBJCOPY=avr-objcopy
SIZE=avr-size
AVRDUDE=avrdude
# Особиисто в мене з avrdude були проблеми тож я використовував той що йшов з Arduino IDE в комплекті
# AVRDUDE=/home/aurelius/.arduino15/packages/MicroCore/tools/avrdude/7.1-arduino.1/bin/avrdude
TARGET=main

# Налаштування файлів соурс коду
ifdef DEBUG
SRCS=main.c ../lib/uart.c
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -DDEBUG=${DEBUG} -I.
else
SRCS=main.c
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I.
endif

# Тип прогаматора. Для ArduinoAsISP використовується stk500v1
PROGRAMMER=stk500v1
# Порт до якого підєднаний програматор
PORT=/dev/ttyACM0

# Компіляція, лінкування тд тп
cmp:
	${CC} ${CFLAGS} -o ${TARGET}.o ${SRCS}
	${LD} -o ${TARGET}.elf ${TARGET}.o
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o ${TARGET}.hex
	${SIZE} -C --mcu=${MCU} ${TARGET}.elf

# Прошивка
flash:
	${AVRDUDE} -v -V -b19200 -p${MCU} -c${PROGRAMMER} -P${PORT} -Uflash:w:${TARGET}.hex:i

# Прошивка фьюзів.
fuse:
	$(AVRDUDE) -v -V -b19200 -p${MCU} -c${PROGRAMMER} -P${PORT} -Uhfuse:w:${FUSE_H}:m -Ulfuse:w:${FUSE_L}:m

# Надання пермішену на використання порту для прошивки
perm:
	sudo chmod a+rw ${PORT}

# Очистка файлів згенерованих при компіляції
clean:
	rm -f *.c~ *.h~ *.o *.elf *.hex