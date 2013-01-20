#include <assert.h>
#include "opcodes.h"

char *nmemonics[] = 
{
"or",   "and",  "nor",  "xor",
"add",  "sub",  "mul",  "div",
"uadd", "usub", "umul", "udiv",
"shl",  "shr",  "shra", "wshr",
"inci",
"deci",
"andi",
"ori",
"xori",
"lui", 
"cpop",
"shli",
"shri",
"shrai",
"ba",
"bal",
"bz",
"bnz",
"jr",
"jlr",
"ldw",
"stw",
"ll",
"sc",
"ldcp",
"stcp",
"int",
"brk",
"lrpc",
"rfi",
};

int main(int argc, char *argv[])
{
	int i;
	int opcode, subcode;
	optype_t type, newtype;
	char *nmemonic;

	for(i = 0; i < sizeof(nmemonics) / sizeof(nmemonics[0]); i++) {
		assert(resolve_nmemonic(nmemonics[i], &type, &opcode, &subcode) == 0);
		assert(resolve_opcode(opcode, subcode, &nmemonic, &newtype) == 0);
		assert(strcmp(nmemonic, nmemonics[i]) == 0);
		assert(type == newtype);
	}
	return 0;
}
