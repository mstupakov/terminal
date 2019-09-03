terminal: terminal.c
	${CROSS_COMPILE}gcc -o $@ $^

clean:
	-rm -rf terminal

