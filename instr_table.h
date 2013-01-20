#ifndef INSTR_TABLE_H
#define INSTR_TABLE_H

#define OPCODE_STARTBIT 11
#define OPCODE_NUMBITS 5 
#define SUBCODE_STARTBIT 0
#define SUBCODE_NUMBITS 2
/*format:
   r: 0-7
   c: 0-7
   x: 0-31
   q: 0-31
   s#: signed #-bit number
   u#: unsigned #-bit number
*/

typedef struct instr_spec
{
	char *nmemonic;
	int opcode;
	int has_subcode;
	int subcode;
	char *format;
	char *syntax;
} instr_spec_t;

extern instr_spec_t instr_table[];
extern int instr_table_len;
#endif
