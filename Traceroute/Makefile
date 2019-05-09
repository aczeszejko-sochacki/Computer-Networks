# Aleksander Czeszejko-Sochacki 297438

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

traceroute: main.o send.o receive.o packet_const.h
	gcc main.o send.o receive.o -o traceroute

main.o: main.c
	gcc -c main.c

send.o: send.c
	gcc -c send.c

receive.o: receive.c
	gcc -c receive.c

clean:
	rm -f *.o

distclean:
	rm -f traceroute