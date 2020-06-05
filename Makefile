terminal: terminal.c
	${CROSS_COMPILE}gcc -o $@ $^

install: terminal
	cp terminal /usr/bin/

clean:
	-rm -rf terminal

