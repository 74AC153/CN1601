#ifndef UTILS_H
#define UTILS_H

#include "stdint.h"

static inline
uint16_t bitmask16(int start, int num)
{
	return ((1 << num) - 1) << start;
}

static inline
uint16_t field_getbits16(uint16_t field, int start, int num)
{
	uint16_t mask;
	mask = bitmask16(start, num);
	field >>= start;
	field &= mask;
	return field;
}

static inline
uint16_t field_setbits16(uint16_t field, int start, int num, uint16_t val)
{
	uint16_t mask;
	mask = bitmask16(start, num);
	field &= ~mask;
	val <<= start;
	val &= mask;
	field |= val;
	return field;
}

int load_file(char *path, char **data, size_t *datalen);

#endif
