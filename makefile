
home_iot: home_iot.c
	gcc -Wall home_iot.c -o home_iot

user_console: user_console.c
	gcc -Wall user_console.c -o user_console

sensor: sensor.c
	gcc -Wall sensor.c -o sensor
