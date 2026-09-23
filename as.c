/* pezas - assembler for the pez stack machine */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "pez.h"
#include "util.h"

#define MAXLINES 4096
#define MAXLINE   256
#define MAXLABEL   64
#define MAXSYM    256

typedef enum {
	OPR_NONE,
	OPR_REG,
	OPR_IMM,
	OPR_LABEL,
} OperKind;

typedef struct {
	char    name[MAXLABEL];
	int64_t addr;
} Sym;

static void readlines(const char *inpath);
static void stripcomment(char *s);
static char *trim(char *s);
static int islabel(const char *s);
static void splitinstr(char *s, char **mnem, char **operand);
static int findop(const char *inpath, int lineno, const char *name);
static int parsereg(const char *inpath, int lineno, const char *s);
static int64_t parseimm(const char *inpath, int lineno, const char *s);
static void addsym(const char *inpath, int lineno, const char *name, int64_t addr);
static int64_t symaddr(const char *inpath, int lineno, const char *name);
static void assemble(const char *inpath, const char *outpath);
static void usage(void);

char *argv0;

static const struct {
	char     *name;
	Op        op;
	OperKind  kind;
} ops[] = {
	{"PUSH",  OP_PUSH,  OPR_REG},
	{"POP",   OP_POP,   OPR_REG},
	{"PUSHI", OP_PUSHI, OPR_IMM},
	{"JMP",   OP_JMP,   OPR_LABEL},
	{"CMP",   OP_CMP,   OPR_NONE},
	{"JIEZ",  OP_JIEZ,  OPR_LABEL},
	{"JIGZ",  OP_JIGZ,  OPR_LABEL},
	{"JILZ",  OP_JILZ,  OPR_LABEL},
	{"JGEZ",  OP_JGEZ,  OPR_LABEL},
	{"JLEZ",  OP_JLEZ,  OPR_LABEL},
	{"ADD",   OP_ADD,   OPR_NONE},
	{"SUB",   OP_SUB,   OPR_NONE},
	{"MUL",   OP_MUL,   OPR_NONE},
	{"DIV",   OP_DIV,   OPR_NONE},
	{"REM",   OP_REM,   OPR_NONE},
	{"HALT",  OP_HALT,  OPR_NONE},
};

static char lines[MAXLINES][MAXLINE];
static int nlines;

static Sym syms[MAXSYM];
static int nsym;

static void
readlines(const char *inpath)
{
	FILE *fp;
	size_t len;

	if (!(fp = fopen(inpath, "r")))
		die("%s:", inpath);

	nlines = 0;
	while (fgets(lines[nlines], MAXLINE, fp)) {
		len = strlen(lines[nlines]);
		if (len > 0 && lines[nlines][len-1] == '\n')
			lines[nlines][len-1] = '\0';
		if (++nlines >= MAXLINES)
			die("%s: too many lines (max %d)", inpath, MAXLINES);
	}

	fclose(fp);
}

static void
stripcomment(char *s)
{
	char *p;

	if ((p = strchr(s, ';')))
		*p = '\0';
}

static char *
trim(char *s)
{
	char *end;

	while (*s == ' ' || *s == '\t')
		s++;

	if (*s == '\0')
		return s;

	end = s + strlen(s) - 1;
	while (end > s && (*end == ' ' || *end == '\t' || *end == '\r'))
		end--;
	end[1] = '\0';

	return s;
}

static int
islabel(const char *s)
{
	size_t len, i;

	len = strlen(s);
	if (len == 0 || s[len-1] != ':')
		return 0;

	if (!isalpha((unsigned char)s[0]) && s[0] != '_')
		return 0;

	for (i = 1; i < len-1; i++)
		if (!isalnum((unsigned char)s[i]) && s[i] != '_')
			return 0;

	return 1;
}

static void
splitinstr(char *s, char **mnem, char **operand)
{
	char *p;

	*mnem = s;
	p = s;
	while (*p && *p != ' ' && *p != '\t')
		p++;

	if (*p) {
		*p++ = '\0';
		while (*p == ' ' || *p == '\t')
			p++;
		*operand = p;
	} else {
		*operand = NULL;
	}
}

static int
findop(const char *inpath, int lineno, const char *name)
{
	size_t i;

	for (i = 0; i < sizeof(ops)/sizeof(ops[0]); i++)
		if (strcmp(ops[i].name, name) == 0)
			return (int)i;

	die("%s:%d: unknown mnemonic '%s'", inpath, lineno, name);
	return -1; /* NOTREACHED */
}

static int
parsereg(const char *inpath, int lineno, const char *s)
{
	char *digits, *end;
	long v;

	if (!s || s[0] != 'r')
		die("%s:%d: expected register operand", inpath, lineno);

	digits = (char *)s + 1;
	v = strtol(digits, &end, 10);
	if (*end != '\0' || end == digits || v < 0 || v >= NREG)
		die("%s:%d: invalid register '%s'", inpath, lineno, s);

	return (int)v;
}

static int64_t
parseimm(const char *inpath, int lineno, const char *s)
{
	char *end;
	long long v;

	if (!s || *s == '\0')
		die("%s:%d: expected immediate operand", inpath, lineno);

	v = strtoll(s, &end, 10);
	if (*end != '\0' || end == s)
		die("%s:%d: invalid immediate '%s'", inpath, lineno, s);

	return (int64_t)v;
}

static void
addsym(const char *inpath, int lineno, const char *name, int64_t addr)
{
	int i;

	for (i = 0; i < nsym; i++)
		if (strcmp(syms[i].name, name) == 0)
			die("%s:%d: duplicate label '%s'", inpath, lineno, name);

	if (nsym >= MAXSYM)
		die("%s:%d: too many labels (max %d)", inpath, lineno, MAXSYM);
	if (strlen(name) >= sizeof(syms[0].name))
		die("%s:%d: label too long '%s'", inpath, lineno, name);

	strcpy(syms[nsym].name, name);
	syms[nsym].addr = addr;
	nsym++;
}

static int64_t
symaddr(const char *inpath, int lineno, const char *name)
{
	int i;

	for (i = 0; i < nsym; i++)
		if (strcmp(syms[i].name, name) == 0)
			return syms[i].addr;

	die("%s:%d: undefined label '%s'", inpath, lineno, name);
	return 0; /* NOTREACHED */
}

static void
assemble(const char *inpath, const char *outpath)
{
	FILE *fp;
	Header hdr;
	Instr instrs[MAXLINES];
	int64_t addr;
	int i, ninstr, opidx;
	char buf[MAXLINE], *s, *mnem, *operand;

	readlines(inpath);

	/* pass 1: record label addresses */
	addr = 0;
	nsym = 0;
	for (i = 0; i < nlines; i++) {
		strncpy(buf, lines[i], sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		stripcomment(buf);
		s = trim(buf);

		if (*s == '\0')
			continue;

		if (islabel(s)) {
			s[strlen(s)-1] = '\0';
			addsym(inpath, i+1, s, addr);
			continue;
		}

		addr++;
	}

	/* pass 2: parse instructions, resolving label references */
	ninstr = 0;
	for (i = 0; i < nlines; i++) {
		strncpy(buf, lines[i], sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		stripcomment(buf);
		s = trim(buf);

		if (*s == '\0' || islabel(s))
			continue;

		splitinstr(s, &mnem, &operand);
		opidx = findop(inpath, i+1, mnem);

		instrs[ninstr].op = ops[opidx].op;
		instrs[ninstr].reg = 0;
		instrs[ninstr].imm = 0;

		switch (ops[opidx].kind) {
		case OPR_REG:
			instrs[ninstr].reg = parsereg(inpath, i+1, operand);
			break;
		case OPR_IMM:
			instrs[ninstr].imm = parseimm(inpath, i+1, operand);
			break;
		case OPR_LABEL:
			if (!operand || *operand == '\0')
				die("%s:%d: expected label operand", inpath, i+1);
			instrs[ninstr].imm = symaddr(inpath, i+1, operand);
			break;
		case OPR_NONE:
			break;
		}

		ninstr++;
	}

	if (!(fp = fopen(outpath, "wb")))
		die("%s:", outpath);

	memcpy(hdr.magic, PEZ_MAGIC, 4);
	hdr.ninstr = ninstr;
	if (fwrite(&hdr, sizeof(hdr), 1, fp) != 1)
		die("%s: write failed", outpath);

	for (i = 0; i < ninstr; i++) {
		EncInstr e;

		memset(&e, 0, sizeof(e));
		e.op = (uint8_t)instrs[i].op;
		e.reg = (uint8_t)instrs[i].reg;
		e.imm = instrs[i].imm;

		if (fwrite(&e, sizeof(e), 1, fp) != 1)
			die("%s: write failed", outpath);
	}

	fclose(fp);
}

static void
usage(void)
{
	die("usage: %s -i source.pez -o out", argv0);
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

	assemble(inpath, outpath);

	return 0;
}
