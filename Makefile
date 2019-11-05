make:
	gcc noncanonical.c connection.c application.c utils/state_machine.c utils/builders.c utils/strmanip.c -Wall -pedantic -o noncanonical -fno-stack-protector
