target: main 


gc.o:	gc.c
	gcc -Wall -c gc.c

at.o:	at.c
	gcc -Wall -c at.c

vm.o:	vm.c
	gcc -Wall -c vm.c

ad.o:	ad.c	vm.o
	gcc -Wall -c ad.c vm.o

parser.o:	parser.c
	gcc -Wall -c parser.c

utils.o:	utils.c
	gcc -Wall -c utils.c

lexer.o:	lexer.c	utils.o
	gcc -Wall -c lexer.c 


main:	main.c	lexer.o parser.o	ad.o	at.o	vm.o	gc.o
	gcc -Wall -o main lexer.o utils.o parser.o ad.o at.o vm.o gc.o main.c

