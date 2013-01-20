#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

enum optype { OP3, OP2U, OP2S, OP1U, OP1S, OP0U, OP0S };
typedef enum optype optype_t;


enum opname {
	OP_BIT,
	OP_MATH,
	OP_SHIFT,
	OP_LI,
	OP_SHLLI,
	OP_INCI,
	OP_DECI,
	OP_ORI,
	OP_ANDI,
	OP_XORI,
	OP_SHLI,
	OP_SHRI,
	OP_SHRAI,
	OP_LHSWAP,
	OP_BA,
	OP_BAL,
	OP_BZ,
	OP_BNZ,
	OP_JR,
	OP_JLR,
	OP_LDW,
	OP_STW,
	OP_SC,
	OP_MFCP,
	OP_MTCP,
	OP_CPOP,
	OP_INT,
	OP_RFI,
	OP_MFCTL,
	OP_MTCTL
};
typedef enum opname opname_t;

enum subname {
	SN_OR = 0,
	SN_NOT,
	SN_AND,
	SN_XOR,
	SN_ADD = 0,
	SN_SUB,
	SN_ADDC,
	SN_SUBC,
	SN_SHL = 0,
	SN_SHR,
	SN_SHRA,
};
typedef enum subname subname_t;

/* nmemonic -> type, opcode, subcode */
int resolve_nmemonic(
	char *nmemonic, // in
	optype_t *type, // out
	unsigned int *opcode, // out
	unsigned int *subcode); // out

/* opcode, subcode -> nmemonic, type */
int resolve_opcode(
	unsigned int opcode, // in
	unsigned int subcode, // in
	char **nmemonic, // out
	optype_t *type); // out

enum fieldtype { U3, S5, U5, S8, U8, S11, U11 };
typedef enum fieldtype fieldtype_t;

int would_safe_trunc(
	fieldtype_t ftype, // in
	int val); // in

int detrunc_immed(
	optype_t type, // in
	unsigned int val); // in

unsigned int trunc_immed(
	optype_t type, // in
	int val); // in

/* instr word -> type, opcode, arg1, arg2, arg3, subtype */
void decode_instr(
	uint16_t instr,
	optype_t *type,
	int *opcode,
	int *arg1,
	int *arg2,
	int *arg3,
	int *subtype);

/* opcode, arg1, arg2, arg3, subtype -> instr word */
void encode_instr(
	int opcode,
	int arg1,
	int arg2,
	int arg3,
	int subtype,
	uint16_t *instr);



#endif
