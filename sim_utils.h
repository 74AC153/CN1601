#ifndef SIM_UTILS_H
#define SIM_UTILS_H

#include <stdint.h>

/* convert array of bits into single value */
void mux_lines(uint8_t **lines, int nlines, uint32_t *outval);

/* convert single value into array of bits */
void demux_lines(uint32_t inval, uint8_t **lines, int nlines);

#endif
