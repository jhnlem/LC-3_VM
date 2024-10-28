#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX];

enum {
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,
	R_R8,
	R_PC,
	R_COND,
	R_COUNT,
};

uint16_t reg[R_COUNT];

enum {
	OP_BR = 0,
	OP_ADD,
	OP_LD,
	OP_ST,
	OP_JSR,
	OP_AND,
	OP_LDR,
	OP_STR,
	OP_RTI,
	OP_NOT,
	OP_LDI,
	OP_STI,
	OP_JMP,
	OP_RES,
	OP_LEA,
	OP_TRAP,
};

enum {
	FL_POS = 1 << 0,
	FL_ZRO = 1 << 1,
	FL_NEG = 1 << 2,
};

int main(int argc, const char* argv[])
{
	if (argc < 2) {
		printf("lc3 [image-file1] ...\n");
		exit(2);
	}

	for (int j = 1; j < argc; ++j) {
		if (!read_image(argv[j])) {
			printf("failed to lad image: %s\n", argv[j]);
			exit(1);
		}
	}

	signal(SIGINT, handle_interrupt);
	disable_input_buffering();

	reg[R_COND] = FL_ZRO;

	enum {PC_START = 0x3000};
	reg[R_PC] = PC_START;

	int running = 1;
	while (running) {
		uint16_t instr = mem_read(reg[R_PC]++);
		uint16_t op = instr >> 12;

		switch (op) {
		case OP_ADD:
			break;
		case OP_AND:
			break;
		case OP_NOT:
			break;
		case OP_BR:
			break;
		case OP_JMP:
			break;
		case OP_JSR:
			break;
		case OP_LD:
			break;
		case OP_LDI:
			break;
		case OP_LDR:
			break;
		case OP_LEA:
			break;
		case OP_ST:
			break;
		case OP_STI:
			break;
		case OP_STR:
			break;
		case OP_TRAP:
			break;
		case OP_RES:
		case OP_RTI:
		default:
			break;
		}
	}
	restore_input_buffering();
}
