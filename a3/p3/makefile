.phony all:
all:
	gcc -Wall a3functions.c diskinfo.c -o diskinfo
	gcc -Wall a3functions.c queue.c disklist.c -o disklist -lm
	gcc -Wall a3functions.c diskget.c -o diskget
	gcc -Wall a3functions.c queue.c diskput.c -o diskput -lm



.PHONY clean:
clean:
	-rm -rf *.o *.exe

