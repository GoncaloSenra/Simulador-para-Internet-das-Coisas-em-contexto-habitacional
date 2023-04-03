
home_iot:
	gcc -Wall home_iot.c -o home_iot

user_console:
	gcc -Wall user_console.c -o user_console

sensor: prog3.o utils.o
	gcc -Wall sensor.c -o sensor

.PHONY: all home_iot clean