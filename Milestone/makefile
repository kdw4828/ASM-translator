compile: parser.o symbol_table.o ast-print.o
	gcc -o compile driver.c scanner.c parser.o symbol_table.o ast-print.o

parser.o: parser.c code_gen.h
	gcc -c parser.c

symbol_table.o: symbol_table.c symbol_table.h
	gcc -c symbol_table.c

ast-print.o: ast-print.c
	gcc -c ast-print.c

clean:
	rm -f *.o compile
