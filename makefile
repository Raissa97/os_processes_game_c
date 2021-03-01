CC = gcc -c
CFLAGS = -std=c89 -pedantic 

progettoSO: master.o player.o pawns.o
	gcc $(CFLAGS) -o master master.c -D_XOPEN_SOURCE=700

master.o: master.c master.h global.h player.c 
	$(CC) master.c 
	
player.o: player.c pawns.c 
	gcc $(CFLAGS) -o player player.c

pawns.o: pawns.c 
	gcc $(CFLAGS) -o pawns pawns.c

 
clean:
	rm -f *.o master player pawns 
	ipcrm -a
	clear
	
run: master
	./master

#PER COMPILARE:
#make 
#
#


#PER ESEGUIRE:
#make run

#PER RIMUOVERE TUTTI I FILE OGGETTO:
#make clean


# ps -e --sort=pid | grep pawn