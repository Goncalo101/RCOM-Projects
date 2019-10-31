make:
	gcc noncanonical.c connection.c application.c utils/state_machine.c utils/builders.c utils/strmanip.c -o noncanonical -Wall -pedantic
