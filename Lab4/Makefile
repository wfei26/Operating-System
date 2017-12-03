all:
	gcc -g client.c -o client -lm
	gcc -g server.c -o server -lm

clean:
	rm -f client server mysock

tar:
	mkdir eecs678-lab04
	cp client.c server.c Makefile eecs678-lab04/
	tar cvzf eecs678-lab04.tar.gz eecs678-lab04
	rm -rf eecs678-lab04

