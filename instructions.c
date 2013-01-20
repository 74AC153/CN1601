#include <stdarg.h>
#include <strings.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "instructions.h"

#define BITMASK(WIDTH, START) \
	( ( (0x1 << (WIDTH)) - 1) << (START) )

#define SGNEXTEND(VAL, TYPE, BITNUM) \
	( (TYPE) ( (VAL) << ((sizeof(VAL) * CHAR_BIT) - BITNUM) ) \
	                 >> ((sizeof(VAL) * CHAR_BIT) - BITNUM)     )
int nmemonic_syntax(
	char **syntax,  /* out */
	char *nmemonic) /* in */
{
	int i;
	for(i = 0; i < instr_table_len; i++) {
		if(strcasecmp(nmemonic, instr_table[i].nmemonic) == 0) {
			*syntax = instr_table[i].syntax;
			return 0;
		}
	}
	return -1;
}

int assemble_instruction(
	uint16_t *instr, /* out */
	char *nmemonic,  /* in */
	int *fields)      /* in */
{
	int blank, numbits, val;
	uint16_t instr_local;
	char *format;
	int i;

	for(i = 0; i < instr_table_len; i++) {
		if(strcasecmp(nmemonic, instr_table[i].nmemonic) == 0) {
			format = instr_table[i].format;
			break;
		}
	}
	if(i == instr_table_len) {
		return -1;
	}
	
	/* every instuction has 5 bit opcode for high bits */
	instr_local = instr_table[i].opcode;

	while(*format) {
		blank = 0;
		if(*format++ == '_') {
			blank = 1;
		}
		assert(*format);

		val = blank ? 0 : *fields++;
		numbits = strtol(format, &format, 10);
		instr_local <<= numbits;
		instr_local |= val & BITMASK(numbits, 0);
	}

	if(instr_table[i].has_subcode) {
		instr_local <<= 2;
		instr_local |= instr_table[i].subcode;
	}

	*instr = instr_local;
	return 0;
}

int resolve_nmemonic(
	char **nmemonic, /* out */
	int opcode,     /* in */
	int subcode)    /* in */
{
	int i;

	for(i = 0; i < instr_table_len && instr_table[i].opcode != opcode; i++);
	assert(i != instr_table_len);
	if(instr_table[i].has_subcode) {
		for( ; i < instr_table_len && instr_table[i].subcode != subcode; i++);
		assert(i != instr_table_len);
	}

	*nmemonic = instr_table[i].nmemonic;
	return 0;
}

typedef enum {
	FMT_INVALID,
	FMT_u3u3u3u2,
	FMT_u3_2u4u2,
	FMT_u3u8,
	FMT_u3_8,
	FMT__11,
	FMT_s11,
	FMT_u3s8,
	FMT_u3u3s5,
	FMT_u3u3u5,
	FMT_u3u3_5,
	FMT_u11,
	FMT_u3_3u5
} instr_fmt_t;

int decode_instruction(
	int *opcode,    /* out */
	int *subcode,   /* out */
	int *fields,    /* out */
	uint16_t instr) /* in */
{
	instr_fmt_t fmt = FMT_INVALID;
	*opcode = (instr >> OPCODE_STARTBIT) & BITMASK(OPCODE_NUMBITS, 0);

	switch(*opcode) {
		case 0x00:
		case 0x01:
		case 0x02: fmt = FMT_u3u3u3u2; break;
		case 0x03: fmt = FMT_u3_2u4u2; break;
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A: 
		case 0x0B:
		case 0x0C:
		case 0x0D: fmt = FMT_u3u8; break;
		case 0x0E: fmt = FMT__11; break;
		case 0x0F:
		case 0x10: fmt = FMT_s11; break;
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14: fmt = FMT_u3s8; break;
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18: fmt = FMT_u3u3s5; break;
		case 0x19:
		case 0x1A: fmt = FMT_u3u3u5; break;
		case 0x1B: fmt = FMT_u3u3_5; break;
		case 0x1C: fmt = FMT_u11; break;
		case 0x1D: fmt = FMT__11; break;
		case 0x1E:
		case 0x1F: fmt = FMT_u3_3u5; break;
		default: break;
	}

	switch(fmt) {
	case FMT_u3u3u3u2:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = (instr >> 5) & 0x7;
		fields[2] = (instr >> 2) & 0x7;
		*subcode = (instr) & 0x3;
		break;
	case FMT_u3_2u4u2:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = (instr >> 2) & 0xF;
		fields[2] = 0;
		*subcode = (instr) & 0x3;
		break;
	case FMT_u3u8:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = instr & 0xFF;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_u3_8:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = 0;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT__11:
		fields[0] = 0;
		fields[1] = 0;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_s11:
		fields[0] = SGNEXTEND(instr & 0x7FF, int32_t, 11);
		fields[1] = 0;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_u3s8:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = SGNEXTEND(instr & 0xFF, int32_t, 8);
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_u3u3s5:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = (instr >> 5) & 0x7;
		fields[2] = SGNEXTEND(instr & 0x1F, int32_t, 5);
		*subcode = 0;
		break;
	case FMT_u3u3u5:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = (instr >> 5) & 0x7;
		fields[2] = instr & 0x1F;
		*subcode = 0;
		break;
	case FMT_u3u3_5:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = (instr >> 5) & 0x7;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_u11:
		fields[0] = (instr & 0x7FF);
		fields[1] = 0;
		fields[2] = 0;
		*subcode = 0;
		break;
	case FMT_u3_3u5:
		fields[0] = (instr >> 8) & 0x7;
		fields[1] = instr & 0x1F;
		fields[2] = 0;
		*subcode = 0;
		break;
	default:
		return -1;
	}

	return 0;
}
