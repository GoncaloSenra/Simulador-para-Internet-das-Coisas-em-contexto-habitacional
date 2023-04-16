
all: home_iot.c user_console.c sensor.c
	gcc -Wall -pthread -fcommon home_iot.c worker.c alerts_watcher.c sys_header.h -o home_iot
	gcc -Wall user_console.c -o user_console
	gcc -Wall sensor.c -o sensor

home_iot: home_iot.c
	gcc -Wall -pthread -fcommon home_iot.c worker.c alerts_watcher.c sys_header.h -o home_iot

user_console: user_console.c
	gcc -Wall user_console.c -o user_console

sensor: sensor.c
	gcc -Wall sensor.c -o sensor
