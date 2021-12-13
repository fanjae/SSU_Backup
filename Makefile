ssu_backup: ssu_backup.o ssu_file.o
	gcc -g -o ssu_backup ssu_backup.o ssu_file.o -lpthread

ssu_backup.o: ssu_file.h ssu_backup.c
	gcc -c -o ssu_backup.o ssu_backup.c -lpthread

ssu_file.o: ssu_file.h ssu_file.c
	gcc -c -o ssu_file.o ssu_file.c
