#include <stdlib.h>
#include <limits.h>
#include "opcodes.h"

struct opcodedecl {
	char *nmemonic;
	opname_t opcode;
	subname_t subcode;
	optype_t type;
};
typedef struct opcodedecl opcodedecl_t;

opcodedecl_t opcodes[] = {
	{ "or",    OP_BIT,   SN_OR,   OP3  },
	{ "not",   OP_BIT,   SN_NOT,  OP3  },
	{ "and",   OP_BIT,   SN_AND,  OP3  },
	{ "xor",   OP_BIT,   SN_XOR,  OP3  },
	{ "add",   OP_MATH,  SN_ADD,  OP3  },
	{ "sub",   OP_MATH,  SN_SUB,  OP3  },
	{ "addc",  OP_MATH,  SN_ADDC, OP3  },
	{ "subc",  OP_MATH,  SN_SUBC, OP3  },
	{ "shl",   OP_SHIFT, SN_SHL,  OP3  },
	{ "shr",   OP_SHIFT, SN_SHR,  OP3  },
	{ "shra",  OP_SHIFT, SN_SHRA, OP3  },
	{ "li",    OP_LI,    0,       OP1U },
	{ "shlli", OP_SHLLI, 0,       OP1U },
	{ "inci",  OP_INCI,  0,       OP1U },
	{ "deci",  OP_DECI,  0,       OP1U },
	{ "ori",   OP_ORI,   0,       OP1U },
	{ "andi",  OP_ANDI,  0,       OP1U },
	{ "xori",  OP_XORI,  0,       OP1U },
	{ "shli",  OP_SHLI,  0,       OP1U },
	{ "shri",  OP_SHRI,  0,       OP1U },
	{ "shrai", OP_SHRAI, 0,       OP1U },
	{ "lhswp", OP_LHSWP, 0,       OP1U },
	{ "ba",    OP_BA,    0,       OP0S },
	{ "bal",   OP_BAL,   0,       OP0S },
	{ "bz",    OP_BZ,    0,       OP1S },
	{ "bnz",   OP_BNZ,   0,       OP1S },
	{ "jr",    OP_JR,    0,       OP1S },
	{ "jlr",   OP_JLR,   0,       OP1S },
	{ "ldw",   OP_LDW,   0,       OP2S },
	{ "stw",   OP_STW,   0,       OP2S },
	{ "sc",    OP_SC,    0,       OP2S },
	{ "mfcp",  OP_LDCP,  0,       OP2U },
	{ "mtcp",  OP_STCP,  0,       OP2U },
	{ "cpop",  OP_CPOP,  0,       OP1U },
	{ "int",   OP_INT,   0,       OP0U },
	{ "rfi",   OP_RFI,   0,       OP0U }
	{ "mfctl", OP_MFCTL, 0,       OP1U },
	{ "mtctl", OP_MTCTL, 0,       OP1U },
};

int resolve_nmemonic(
	char *nmemonic, // in
	optype_t *type, // out
	unsigned int *opcode, // out
	unsigned int *subcode) // out
{
	int i;
	for(i = 0; i < sizeof(opcodes) / sizeof(opcodes[0]); i++) {
		if(strcmp(opcodes[i].nmemonic, nmemonic) == 0) {
			*type = opcodes[i].type;
			*opcode = opcodes[i].opcode;
			*subcode = opcodes[i].subcode;
			break;
		}
	}

	if(i == sizeof(opcodes) / sizeof(opcodes[0])) {
		return -1;
	}

	return 0;
}

int resolve_opcode(
	unsigned int opcode, // in
	unsigned int subcode, // in
	char **nmemonic, // out
	optype_t *type) // out
{
	int i;
	for(i = 0; i < sizeof(opcodes) / sizeof(opcodes[0]); i++) {
		if(opcodes[i].opcode == opcode) {
			if(type) {
				*type = opcodes[i].type;
			}
			if(opcodes[i].subcode == subcode) {
				if(nmemonic) {
					*nmemonic = opcodes[i].nmemonic;
				}
			}
		}
	}

	if(i == sizeof(opcodes) / sizeof(opcodes[0])) {
		return -1;
	}

	return 0;
}


int detrunc_immed(optype_t type, unsigned int val)
{
	int ret = (int) val;
	int m;
	switch(type) {
		case OP2S: /* 5 bits, signed */
			m = CHAR_BIT * sizeof(int) - 5;
			return (ret << m) >> m;
		case OP2U: /* 5 bits, unsigned */
			return ret & ((0x1U << 5) - 1);
		case OP1S: /* 8 bits, signed */
			m = CHAR_BIT * sizeof(int) - 8;
			return (ret << m) >> m;
		case OP1U: /* 8 bits, unsigned */
			return ret & ((0x1U << 8) - 1);
		case OP0S: /* 11 bits signed */
			m = CHAR_BIT * sizeof(int) - 11;
			return (ret << m) >> m;
		case OP0U: /* 11 bits, unsigned */
			return ret & ((0x1U << 11) - 1);
		default: break;
	}

	return 0;
}


int would_safe_trunc( fieldtype_t ftype, int val)
{
	switch(ftype) {
		case U3:
			return val >= 0 && val < 8;
		case S5:
			return val >= -16 && val < 16;
		case U5:
			return val >= 0 && val < 32;
		case S8:
			return val >= -128 && val < 128;
		case U8:
			return val >= 0 && val < 256;
		case S11:
			return val >= -1024 && val < 1024;
		case U11:
			return val >= 0 && val < 2048;
		default:
			return 0;
	}
}

unsigned int trunc_immed(optype_t type, int val)
{
	unsigned int ret = (unsigned int) val;
	switch(type) {
		case OP2S: /* 5 bits, signed */
		case OP2U: /* 5 bits, unsigned */
			return ret & ((0x1U << 5) - 1);

		case OP1S: /* 8 bits, signed */
		case OP1U: /* 8 bits, unsigned */
			return ret & ((0x1U << 8) - 1);

		case OP0S: /* 11 bits signed */
		case OP0U: /* 11 bits, unsigned */
			return ret & ((0x1U << 11) - 1);

		default: break;
	}

	return 0;
}


void decode_instr(
	uint16_t instr,
	optype_t *type,
	int *opcode,
	int *arg1,
	int *arg2,
	int *arg3,
	int *subtype)
{
	*opcode = instr >> 11; 
	resolve_opcode(*opcode, 0, NULL, type);

	switch(*type) {
		case OP3:
			*arg1 = (instr & 0x0700) >> 8;
			*arg2 = (instr & 0x00E0) >> 5;
			*arg3 = (instr & 0x001C) >> 2;
			*subtype = (instr & 0x0003);
			break;
		case OP2U:
			*arg1 = (instr & 0x0700) >> 8;
			*arg2 = (instr & 0x00E0) >> 5;
			*arg3 = detrunc_immed(OP2U, (instr & 0x1F));
			*subtype = 0;
			break;
		case OP2S:
			*arg1 = (instr & 0x0700) >> 8;
			*arg2 = (instr & 0x00E0) >> 5;
			*arg3 = detrunc_immed(OP2S, (instr & 0x1F));
			*subtype = 0;
			break;
		case OP1U:
			*arg1 = (instr & 0x0700) >> 8;
			*arg2 = detrunc_immed(OP1U, (instr & 0xFF));
			*arg3 = 0;
			*subtype = 0;
			break;
		case OP1S:
			*arg1 = (instr & 0x0700) >> 8;
			*arg2 = detrunc_immed(OP1S, (instr & 0xFF));
			*arg3 = 0;
			*subtype = 0;
			break;
		case OP0U:
			*arg1 = detrunc_immed(OP0U, (instr & 0x7FF));
			*arg2 = 0;
			*arg3 = 0;
			*subtype = 0;
			break;
		case OP0S:
			*arg1 = detrunc_immed(OP0S, (instr & 0x7FF));
			*arg2 = 0;
			*arg3 = 0;
			*subtype = 0;
			break;
	}
}

void encode_instr(
	int opcode,
	int arg1,
	int arg2,
	int arg3,
	int subtype,
	uint16_t *instr)
{
	optype_t type;

	resolve_opcode(opcode, 0, NULL, &type);

	switch(type) {
	case OP3:
		*instr = (opcode & 0x1F) << 11 |
		         (arg1 & 0x7) << 8 |
		         (arg2 & 0x7) << 5 |
		         (arg3 & 0x7) << 2 |
		         (subtype & 0x3);
		break;
	case OP2S:
	case OP2U:
		*instr = (opcode & 0x1F) << 11 |
		         (arg1 & 0x7) << 8 |
		         (arg2 & 0x7) << 5 |
		         (arg3 & 0x1f);
		break;
	case OP1S:
	case OP1U:
		*instr = (opcode & 0x1F) << 11 |
		         (arg1 & 0x7) << 8 |
		         (arg2 & 0xFF);
		break;
	case OP0S:
	case OP0U:
		*instr = (opcode & 0x1F) << 11 |
		         (arg1 & 0x7FF);
		break;
	}
}
