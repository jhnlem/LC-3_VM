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

uint16_t sign_extend(uint16_t x, int bit_count)
{
	if ((x >> (bit_count - 1)) & 1) 
		x |= (0xFFFF << bit_count);
	return x;
}

void update_flags(uint16_t r)
{
	if (reg[r] == 0)
		reg[R_COND] = FL_ZRO;
	else if (reg[r] >> 15)
		reg[R_COND] = FL_NEG;
	else
		reg[R_COND] = FL_POS;
}

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
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t r1 = (instr >> 6) & 0x7;
			uint16_t imm_flag = (instr >> 5) & 0x1;

			if (imm_flag) {
				uint16_t imm5 = sign_extend(instr & 0x1F, 5);
				reg[r0] = reg[r1] + imm5;
			} else {
				uint16_t r2 = instr & 0x7;
				reg[r0] = reg[r1] + reg[r2];
			}

			update_flags(r0);
			break;
		case OP_AND:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t r1 = (instr >> 6) & 0x7;
			uint16_t imm_flag = (instr >> 5) & 0x1;

			if (imm_flag) {
				uint16_t imm5 = sign_extend(instr & 0x1F, 5)
				reg[r0] = reg[r1] & imm5;
			} else {
				uint16_t r2 = instr & 0x7;
				reg[r0] = reg[r1] & reg[r2];
			}

			update_flags(r0);
			break;
		case OP_NOT:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t r1 = (instr >> 6) & 0x7;
			reg[r0] = ~reg[r1];
			update_flags(r0);
			break;
		case OP_BR:
			uint16_t br_cond = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			if (reg[R_COND] & br_cond)
				reg[R_PC] += pc_offset;
			break;
		case OP_JMP:
			uint16_t r1 = (instr >> 6) & 0x7;
			reg[R_PC] = reg[r1];
			break;
		case OP_JSR:
			uint16_t imm_flag = (instr >> 11) & 0x1;
			reg[R_R7] = reg[R_PC];

			if (imm_flag) {
				uint16_t pc_offset = sign_extend(instr & 0x7FF, 11);
				reg[R_PC] += pc_offset;
			} else {
				uint16_t r1 = (instr >> 6) & 0x7;
				reg[R_PC] = reg[r1];
			}

			break;
		case OP_LD:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			reg[r0] = mem_read(reg[R_PC] + pc_offset);
			update_flags(r0);
			break;
		case OP_LDI:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
			update_flags(r0);
			break;
		case OP_LDR:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t r1 = (instr >> 6) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x3F, 6);
			reg[r0] = mem_read(reg[r1] + pc_offset);
			update_flags(r0);
			break;
		case OP_LEA:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			reg[r0] = reg[R_PC] + pc_offset;
			update_flags(r0);
			break;
		case OP_ST:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			mem_write(reg[R_PC] + pc_offset, r0);
			break;
		case OP_STI:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
			mem_write(mem_read(reg[R_PC] + pc_offset), r0);
			break;
		case OP_STR:
			uint16_t r0 = (instr >> 9) & 0x7;
			uint16_t r1 = (instr >> 6) & 0x7;
			uint16_t pc_offset = sign_extend(instr & 0x3F, 6);
			mem_write(reg[r1] + pc_offset, r0);
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
