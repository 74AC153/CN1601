#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include "instr_table.h"

int nmemonic_syntax(
	char **syntax,   /* out */
	char *nmemonic); /* in */

int assemble_instruction(
	uint16_t *instr, /* out */
	char *nmemonic,  /* in */
	int *fields);     /* in */

int resolve_nmemonic(
	char **nmemonic, /* out */
	int opcode,     /* in */
	int subcode);   /* in */

int decode_instruction(
	int *opcode,     /* out */
	int *subcode,    /* out */
	int *fields,     /* out */
	uint16_t instr); /* in */

#endif
