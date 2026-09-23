/* pez - shared instruction set definitions for the assembler and VM */
#include <stdint.h>

#define NREG      16     /* r0 .. r15 */
#define WORDSZ     8     /* bytes per word (64-bit) */
#define PEZ_MAGIC "PEZ1" /* 4-byte on-disk file magic, no NUL */

typedef enum {
	OP_PUSH,
	OP_POP,
	OP_PUSHI,
	OP_JMP,
	OP_CALL,
	OP_RET,
	OP_CMP,
	OP_JIEZ,
	OP_JIGZ,
	OP_JILZ,
	OP_JGEZ,
	OP_JLEZ,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_REM,
	OP_HALT,
} Op;

typedef struct instr Instr;
struct instr {
	Op      op;
	int     reg;   /* register operand, unused by ops that don't take one */
	int64_t imm;   /* immediate operand, or resolved jump target address */
};

/*
 * On-disk format: one Header followed by Header.ninstr EncInstr
 * records. Every field is already naturally aligned, so there is no
 * implicit compiler padding and the struct can be read/written with
 * a single fread/fwrite, as long as the assembler and VM are built
 * with the same compiler/ABI (guaranteed here by the shared Makefile).
 */
typedef struct {
	char    magic[4];  /* PEZ_MAGIC */
	int32_t ninstr;    /* number of EncInstr records following */
	int32_t entry;     /* instruction index where execution begins */
} Header;

typedef struct {
	uint8_t op;        /* Op enum value */
	uint8_t reg;       /* register number, 0 if unused */
	uint8_t pad[6];    /* reserved, always zero */
	int64_t imm;       /* immediate, or resolved jump address */
} EncInstr;
