/* pez - stack machine virtual machine */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "pez.h"
#include "util.h"

#define STACKSZ  4096   /* words */
#define MAXINSTR 4096   /* mirrors as.c's MAXLINES program-size cap */

typedef struct {
	int64_t regs[NREG];
	int64_t stack[STACKSZ];
	int     sp;
	int     pc;
} Machine;

static int loadprog(const char *inpath, Instr prog[], int max);
static void push(Machine *m, int64_t v);
static int64_t pop(Machine *m);
static void execute(const Instr *prog, int nprog, Machine *m);
static void dumpstate(const char *outpath, const Machine *m);
static void run(const char *inpath, const char *outpath);
static void usage(void);

char *argv0;

static int
loadprog(const char *inpath, Instr prog[], int max)
{
	FILE *fp;
	Header hdr;
	EncInstr e;
	int i;

	if (!(fp = fopen(inpath, "rb")))
		die("%s:", inpath);

	if (fread(&hdr, sizeof(hdr), 1, fp) != 1)
		die("%s: truncated header", inpath);
	if (memcmp(hdr.magic, PEZ_MAGIC, 4) != 0)
		die("%s: bad magic (not a pez binary)", inpath);
	if (hdr.ninstr < 0 || hdr.ninstr > max)
		die("%s: invalid instruction count %d", inpath, hdr.ninstr);

	for (i = 0; i < hdr.ninstr; i++) {
		if (fread(&e, sizeof(e), 1, fp) != 1)
			die("%s: truncated instruction stream", inpath);
		if (e.op > OP_HALT)
			die("%s: invalid opcode %d at instruction %d", inpath, e.op, i);
		if ((e.op == OP_PUSH || e.op == OP_POP) && e.reg >= NREG)
			die("%s: invalid register %d at instruction %d", inpath, e.reg, i);

		prog[i].op = (Op)e.op;
		prog[i].reg = e.reg;
		prog[i].imm = e.imm;
	}

	fclose(fp);

	return hdr.ninstr;
}

static void
push(Machine *m, int64_t v)
{
	if (m->sp >= STACKSZ)
		die("pc=%d: stack overflow", m->pc);
	m->stack[m->sp++] = v;
}

static int64_t
pop(Machine *m)
{
	if (m->sp <= 0)
		die("pc=%d: stack underflow", m->pc);
	return m->stack[--m->sp];
}

static void
execute(const Instr *prog, int nprog, Machine *m)
{
	Instr in;
	int64_t a, b;

	m->pc = 0;
	for (;;) {
		if (m->pc < 0 || m->pc >= nprog)
			die("pc out of range: %d", m->pc);

		in = prog[m->pc];

		if (in.op == OP_HALT)
			break;

		switch (in.op) {
		case OP_PUSH:
			push(m, m->regs[in.reg]);
			m->pc++;
			break;
		case OP_POP:
			m->regs[in.reg] = pop(m);
			m->pc++;
			break;
		case OP_PUSHI:
			push(m, in.imm);
			m->pc++;
			break;
		case OP_JMP:
			m->pc = (int)in.imm;
			break;
		case OP_CMP:
			a = pop(m);
			b = pop(m);
			push(m, a - b);
			m->pc++;
			break;
		case OP_JIEZ:
			m->pc = (pop(m) == 0) ? (int)in.imm : m->pc + 1;
			break;
		case OP_JIGZ:
			m->pc = (pop(m) > 0) ? (int)in.imm : m->pc + 1;
			break;
		case OP_JILZ:
			m->pc = (pop(m) < 0) ? (int)in.imm : m->pc + 1;
			break;
		case OP_JGEZ:
			m->pc = (pop(m) >= 0) ? (int)in.imm : m->pc + 1;
			break;
		case OP_JLEZ:
			m->pc = (pop(m) <= 0) ? (int)in.imm : m->pc + 1;
			break;
		case OP_ADD:
			a = pop(m);
			b = pop(m);
			push(m, a + b);
			m->pc++;
			break;
		case OP_SUB:
			a = pop(m);
			b = pop(m);
			push(m, a - b);
			m->pc++;
			break;
		case OP_MUL:
			a = pop(m);
			b = pop(m);
			push(m, a * b);
			m->pc++;
			break;
		case OP_DIV:
			a = pop(m);
			b = pop(m);
			if (b == 0)
				die("pc=%d: division by zero", m->pc);
			push(m, a / b);
			m->pc++;
			break;
		case OP_REM:
			a = pop(m);
			b = pop(m);
			if (b == 0)
				die("pc=%d: division by zero", m->pc);
			push(m, a % b);
			m->pc++;
			break;
		default:
			die("pc=%d: invalid opcode %d", m->pc, in.op);
		}
	}
}

static void
dumpstate(const char *outpath, const Machine *m)
{
	FILE *fp;
	int i;

	if (!(fp = fopen(outpath, "w")))
		die("%s:", outpath);

	for (i = 0; i < NREG; i++)
		fprintf(fp, "r%d = %lld\n", i, (long long)m->regs[i]);
	fprintf(fp, "sp = %d\n", m->sp);

	fclose(fp);
}

static void
usage(void)
{
	die("usage: %s -i program -o out", argv0);
}

static void
run(const char *inpath, const char *outpath)
{
	Instr prog[MAXINSTR];
	Machine m;
	int nprog;

	memset(&m, 0, sizeof(m));
	nprog = loadprog(inpath, prog, MAXINSTR);
	execute(prog, nprog, &m);
	dumpstate(outpath, &m);
}

int
main(int argc, char *argv[])
{
	char *inpath, *outpath;

	inpath = NULL;
	outpath = NULL;

	ARGBEGIN {
	case 'i':
		inpath = EARGF(usage());
		break;
	case 'o':
		outpath = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (!inpath || !outpath)
		usage();

	run(inpath, outpath);

	return 0;
}
