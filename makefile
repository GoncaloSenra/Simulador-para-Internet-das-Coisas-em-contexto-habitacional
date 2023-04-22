
all: home_iot.c user_console.c sensor.c
	gcc -Wall -pthread -fcommon home_iot.c worker.c alerts_watcher.c sys_header.h -o home_iot
	gcc -Wall -pthread user_console.c -o user_console
	gcc -Wall -pthread sensor.c -o sensor

home_iot: home_iot.c
	gcc -Wall -pthread -fcommon home_iot.c worker.c alerts_watcher.c sys_header.h -o home_iot

user_console: user_console.c
	gcc -Wall -pthread user_console.c -o user_console

sensor: sensor.c
	gcc -Wall -pthread sensor.c -o sensor
