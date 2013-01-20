#include "sim_utils.h"

void mux_lines(uint8_t **lines, int nlines, uint32_t *outval)
{
	int i;

	*outval = 0;
	for(i = 0; i < nlines; i++) {
		*outval |= (*(lines[i]) & 0x1) << i;
	}
}

void demux_lines(uint32_t inval, uint8_t **lines, int nlines)
{
	int i;

	for(i = 0; i < nlines; i++) {
		*(lines[i]) = (inval >> i) & 0x1;
	}
}
