#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "instr_table.h"
#include "instructions.h"


typedef struct {
	uint16_t instr;
	int opcode;
	int subcode;
	int fields[3];
	char *nmemonic;
} testcase_t;

testcase_t cases[] = {
	{ 0x0000, 0x00, 0, { 0, 0, 0 },     "or"    }, /* or r0, r0, r0 */
	{ 0x0700, 0x00, 0, { 7, 0, 0 },     "or"    }, /* or r7, r0, r0 */
	{ 0x00E0, 0x00, 0, { 0, 7, 0 },     "or"    }, /* or r0, r7, r0 */
	{ 0x001C, 0x00, 0, { 0, 0, 7 },     "or"    }, /* or r0, r0, r7 */
	{ 0x0001, 0x00, 1, { 0, 0, 0 },     "nand"  }, /* nand r0, r0, r0 */
	{ 0x0701, 0x00, 1, { 7, 0, 0 },     "nand"  }, /* nand r7, r0, r0 */
	{ 0x00E1, 0x00, 1, { 0, 7, 0 },     "nand"  }, /* nand r0, r7, r0 */
	{ 0x001D, 0x00, 1, { 0, 0, 7 },     "nand"  }, /* nand r0, r0, r7 */
	{ 0x0002, 0x00, 2, { 0, 0, 0 },     "and"   }, /* and r0, r0, r0 */
	{ 0x0702, 0x00, 2, { 7, 0, 0 },     "and"   }, /* and r7, r0, r0 */
	{ 0x00E2, 0x00, 2, { 0, 7, 0 },     "and"   }, /* and r0, r7, r0 */
	{ 0x001E, 0x00, 2, { 0, 0, 7 },     "and"   }, /* and r0, r0, r7 */
	{ 0x0003, 0x00, 3, { 0, 0, 0 },     "xor"   }, /* xor r0, r0, r0 */
	{ 0x0703, 0x00, 3, { 7, 0, 0 },     "xor"   }, /* xor r7, r0, r0 */
	{ 0x00E3, 0x00, 3, { 0, 7, 0 },     "xor"   }, /* xor r0, r7, r0 */
	{ 0x001F, 0x00, 3, { 0, 0, 7 },     "xor"   }, /* xor r0, r0, r7 */

	{ 0x0800, 0x01, 0, { 0, 0, 0 },     "add"   }, /* add r0, r0, r0 */
	{ 0x0F00, 0x01, 0, { 7, 0, 0 },     "add"   }, /* add r7, r0, r0 */
	{ 0x08E0, 0x01, 0, { 0, 7, 0 },     "add"   }, /* add r0, r7, r0 */
	{ 0x081C, 0x01, 0, { 0, 0, 7 },     "add"   }, /* add r0, r0, r7 */
	{ 0x0801, 0x01, 1, { 0, 0, 0 },     "sub"   }, /* sub r0, r0, r0 */
	{ 0x0F01, 0x01, 1, { 7, 0, 0 },     "sub"   }, /* sub r7, r0, r0 */
	{ 0x08E1, 0x01, 1, { 0, 7, 0 },     "sub"   }, /* sub r0, r7, r0 */
	{ 0x081D, 0x01, 1, { 0, 0, 7 },     "sub"   }, /* sub r0, r0, r7 */
	{ 0x0802, 0x01, 2, { 0, 0, 0 },     "addc"  }, /* addc r0, r0, r0 */
	{ 0x0F02, 0x01, 2, { 7, 0, 0 },     "addc"  }, /* addc r7, r0, r0 */
	{ 0x08E2, 0x01, 2, { 0, 7, 0 },     "addc"  }, /* addc r0, r7, r0 */
	{ 0x081E, 0x01, 2, { 0, 0, 7 },     "addc"  }, /* addc r0, r0, r7 */
	{ 0x0803, 0x01, 3, { 0, 0, 0 },     "subc"  }, /* subc r0, r0, r0 */
	{ 0x0F03, 0x01, 3, { 7, 0, 0 },     "subc"  }, /* subc r7, r0, r0 */
	{ 0x08E3, 0x01, 3, { 0, 7, 0 },     "subc"  }, /* subc r0, r7, r0 */
	{ 0x081F, 0x01, 3, { 0, 0, 7 },     "subc"  }, /* subc r0, r0, r7 */

	{ 0x1000, 0x02, 0, { 0, 0, 0 },     "shl"   }, /* shl r0, r0, r0 */
	{ 0x1700, 0x02, 0, { 7, 0, 0 },     "shl"   }, /* shl r7, r0, r0 */
	{ 0x10E0, 0x02, 0, { 0, 7, 0 },     "shl"   }, /* shl r0, r7, r0 */
	{ 0x101C, 0x02, 0, { 0, 0, 7 },     "shl"   }, /* shl r0, r0, r7 */
	{ 0x1001, 0x02, 1, { 0, 0, 0 },     "shr"   }, /* shr r0, r0, r0 */
	{ 0x1701, 0x02, 1, { 7, 0, 0 },     "shr"   }, /* shr r7, r0, r0 */
	{ 0x10E1, 0x02, 1, { 0, 7, 0 },     "shr"   }, /* shr r0, r7, r0 */
	{ 0x101D, 0x02, 1, { 0, 0, 7 },     "shr"   }, /* shr r0, r0, r7 */
	{ 0x1002, 0x02, 2, { 0, 0, 0 },     "shra"  }, /* shra r0, r0, r0 */
	{ 0x1702, 0x02, 2, { 7, 0, 0 },     "shra"  }, /* shra r7, r0, r0 */
	{ 0x10E2, 0x02, 2, { 0, 7, 0 },     "shra"  }, /* shra r0, r7, r0 */
	{ 0x101E, 0x02, 2, { 0, 0, 7 },     "shra"  }, /* shra r0, r0, r7 */

	{ 0x1800, 0x03, 0, { 0, 0,  0 },    "shli"  }, /* shli r0, #0 */
	{ 0x1F00, 0x03, 0, { 7, 0,  0 },    "shli"  }, /* shli r7, #0 */
	{ 0x183C, 0x03, 0, { 0, 15, 0 },    "shli"  }, /* shli r0, #15 */
	{ 0x1F3C, 0x03, 0, { 7, 15, 0 },    "shli"  }, /* shli r7, #15 */
	{ 0x1801, 0x03, 1, { 0, 0,  0 },    "shri"  }, /* shri r0, #0 */ 
	{ 0x1F01, 0x03, 1, { 7, 0,  0 },    "shri"  }, /* shri r7, #0 */ 
	{ 0x183D, 0x03, 1, { 0, 15, 0 },    "shri"  }, /* shri r0, #15 */
	{ 0x1F3D, 0x03, 1, { 7, 15, 0 },    "shri"  }, /* shri r7, #15 */
	{ 0x1802, 0x03, 2, { 0, 0,  0 },    "shrai" }, /* shrai r0, #0 */ 
	{ 0x1F02, 0x03, 2, { 7, 0,  0 },    "shrai" }, /* shrai r7, #0 */ 
	{ 0x183E, 0x03, 2, { 0, 15, 0 },    "shrai" }, /* shrai r0, #15 */ 
	{ 0x1F3E, 0x03, 2, { 7, 15, 0 },    "shrai" }, /* shrai r7, #15 */ 

	{ 0x2000, 0x04, 0, { 0, 0,   0 },   "li"    }, /* li r0, #0 */
	{ 0x2700, 0x04, 0, { 7, 0,   0 },   "li"    }, /* li r7, #0 */
	{ 0x20FF, 0x04, 0, { 0, 255, 0 },   "li"    }, /* li r0, #255 */
	{ 0x27FF, 0x04, 0, { 7, 255, 0 },   "li"    }, /* li r7, #255 */

	{ 0x2800, 0x05, 0, { 0, 0,   0 },   "shlli" }, /* shlli r0, #0 */
	{ 0x2F00, 0x05, 0, { 7, 0,   0 },   "shlli" }, /* shlli r7, #0 */
	{ 0x28FF, 0x05, 0, { 0, 255, 0 },   "shlli" }, /* shlli r0, #255 */
	{ 0x2FFF, 0x05, 0, { 7, 255, 0 },   "shlli" }, /* shlli r7, #255 */

	{ 0x3000, 0x06, 0, { 0, 0,   0 },   "inci"  }, /* inci r0, #0 */
	{ 0x3700, 0x06, 0, { 7, 0,   0 },   "inci"  }, /* inci r7, #0 */
	{ 0x30FF, 0x06, 0, { 0, 255, 0 },   "inci"  }, /* inci r0, #255 */
	{ 0x37FF, 0x06, 0, { 7, 255, 0 },   "inci"  }, /* inci r7, #255 */

	{ 0x3800, 0x07, 0, { 0, 0,   0 },   "deci"  }, /* deci r0, #0 */
	{ 0x3F00, 0x07, 0, { 7, 0,   0 },   "deci"  }, /* deci r7, #0 */
	{ 0x38FF, 0x07, 0, { 0, 255, 0 },   "deci"  }, /* deci r0, #255 */
	{ 0x3FFF, 0x07, 0, { 7, 255, 0 },   "deci"  }, /* deci r7, #255 */

	{ 0x4000, 0x08, 0, { 0, 0,   0 },   "ori"   }, /* ori r0, #0 */
	{ 0x4700, 0x08, 0, { 7, 0,   0 },   "ori"   }, /* ori r7, #0 */
	{ 0x40FF, 0x08, 0, { 0, 255, 0 },   "ori"   }, /* ori r0, #255 */
	{ 0x47FF, 0x08, 0, { 7, 255, 0 },   "ori"   }, /* ori r7, #255 */

	{ 0x4800, 0x09, 0, { 0, 0,   0 },   "andi"  }, /* andi r0, #0 */
	{ 0x4F00, 0x09, 0, { 7, 0,   0 },   "andi"  }, /* andi r7, #0 */
	{ 0x48FF, 0x09, 0, { 0, 255, 0 },   "andi"  }, /* andi r0, #255 */
	{ 0x4FFF, 0x09, 0, { 7, 255, 0 },   "andi"  }, /* andi r7, #255 */

	{ 0x5000, 0x0A, 0, { 0, 0,   0 },   "xori"  }, /* xori r0, #0 */
	{ 0x5700, 0x0A, 0, { 7, 0,   0 },   "xori"  }, /* xori r7, #0 */
	{ 0x50FF, 0x0A, 0, { 0, 255, 0 },   "xori"  }, /* xori r0, #255 */
	{ 0x57FF, 0x0A, 0, { 7, 255, 0 },   "xori"  }, /* xori r7, #255 */

	/* opcodes 0xB, 0xD, 0xD skipped */

	{ 0x7000, 0x0E, 0, { 0, 0, 0 },     "lhswp" }, /* lhswp r0 */ 
	{ 0x7700, 0x0E, 0, { 7, 0, 0 },     "lhswp" }, /* lhswp r7 */

	{ 0x7800, 0x0F, 0, {     0, 0, 0 }, "ba"    }, /* ba #0 */
	{ 0x7C00, 0x0F, 0, { -1024, 0, 0 }, "ba"    }, /* ba #-1024 */
	{ 0x7BFF, 0x0F, 0, { 1023,  0, 0 }, "ba"    }, /* ba #1023 */

	{ 0x8000, 0x10, 0, { 0,     0, 0 }, "bal"   }, /* bal #0 */
	{ 0x8400, 0x10, 0, { -1024, 0, 0 }, "bal"   }, /* bal #0 */
	{ 0x83FF, 0x10, 0, { 1023,  0, 0 }, "bal"   }, /* bal #0 */

	{ 0x8800, 0x11, 0, { 0, 0,    0 },  "bz"    }, /* bz r0, #0 */
	{ 0x8880, 0x11, 0, { 0, -128, 0 },  "bz"    }, /* bz r0, #-128 */
	{ 0x887F, 0x11, 0, { 0, 127,  0 },  "bz"    }, /* bz r0, #127 */
	{ 0x8F80, 0x11, 0, { 7, -128, 0 },  "bz"    }, /* bz r7, #-128 */
	{ 0x8F7F, 0x11, 0, { 7, 127,  0 },  "bz"    }, /* bz r7, #127 */

	{ 0x9000, 0x12, 0, { 0, 0,    0 },  "bnz"   }, /* bnz r0, #0 */
	{ 0x9080, 0x12, 0, { 0, -128, 0 },  "bnz"   }, /* bnz r0, #-128 */
	{ 0x907F, 0x12, 0, { 0, 127,  0 },  "bnz"   }, /* bnz r0, #127 */
	{ 0x9780, 0x12, 0, { 7, -128, 0 },  "bnz"   }, /* bnz r7, #-128 */
	{ 0x977F, 0x12, 0, { 7, 127,  0 },  "bnz"   }, /* bnz r7, #127 */

	{ 0x9800, 0x13, 0, { 0, 0,    0 },  "jr"    }, /* jr r0, #0 */
	{ 0x9F00, 0x13, 0, { 7, 0,    0 },  "jr"    }, /* jr r7, #0 */
	{ 0x9880, 0x13, 0, { 0, -128, 0 },  "jr"    }, /* jr r0, #-128 */
	{ 0x987F, 0x13, 0, { 0, 127,  0 },  "jr"    }, /* jr r0, #127 */
	{ 0x9F80, 0x13, 0, { 7, -128, 0 },  "jr"    }, /* jr r7, #-128 */
	{ 0x9F7F, 0x13, 0, { 7, 127,  0 },  "jr"    }, /* jr r7, #127 */

	{ 0xA000, 0x14, 0, { 0, 0,    0 },  "jlr"   }, /* jlr r0, #0 */
	{ 0xA700, 0x14, 0, { 7, 0,    0 },  "jlr"   }, /* jlr r7, #0 */
	{ 0xA080, 0x14, 0, { 0, -128, 0 },  "jlr"   }, /* jlr r0, #-128 */
	{ 0xA07F, 0x14, 0, { 0, 127,  0 },  "jlr"   }, /* jlr r0, #127 */
	{ 0xA780, 0x14, 0, { 7, -128, 0 },  "jlr"   }, /* jlr r7, #-128 */
	{ 0xA77F, 0x14, 0, { 7, 127,  0 },  "jlr"   }, /* jlr r7, #127 */


	{ 0xA800, 0x15, 0, { 0, 0, 0   },   "ll"    }, /* ll r0, r0, #0 */
	{ 0xAF10, 0x15, 0, { 7, 0, -16 },   "ll"    }, /* ll r7, r0, #-16 */
	{ 0xA8EF, 0x15, 0, { 0, 7, 15  },   "ll"    }, /* ll r0, r7, #15 */
	{ 0xA8F0, 0x15, 0, { 0, 7, -16 },   "ll"    }, /* ll r0, r7, #-16 */
	{ 0xAF0F, 0x15, 0, { 7, 0, 15  },   "ll"    }, /* ll r7, r0, #15 */

	{ 0xB000, 0x16, 0, { 0, 0, 0   },   "ldw"   }, /* ldw r0, r0, #0 */
	{ 0xB710, 0x16, 0, { 7, 0, -16 },   "ldw"   }, /* ldw r7, r0, #-16 */
	{ 0xB0EF, 0x16, 0, { 0, 7, 15  },   "ldw"   }, /* ldw r0, r7, #15 */
	{ 0xB0F0, 0x16, 0, { 0, 7, -16 },   "ldw"   }, /* ldw r0, r7, #-16 */
	{ 0xB70F, 0x16, 0, { 7, 0, 15  },   "ldw"   }, /* ldw r7, r0, #15 */

	{ 0xB800, 0x17, 0, { 0, 0, 0   },   "stw"   }, /* stw r0, r0, #0 */
	{ 0xBF10, 0x17, 0, { 7, 0, -16 },   "stw"   }, /* stw r7, r0, #-16 */
	{ 0xB8EF, 0x17, 0, { 0, 7, 15  },   "stw"   }, /* stw r0, r7, #15 */
	{ 0xB8F0, 0x17, 0, { 0, 7, -16 },   "stw"   }, /* stw r0, r7, #-16 */
	{ 0xBF0F, 0x17, 0, { 7, 0, 15  },   "stw"   }, /* stw r7, r0, #15 */

	{ 0xC000, 0x18, 0, { 0, 0, 0   },   "sc"    }, /* sc r0, r0, #0 */
	{ 0xC710, 0x18, 0, { 7, 0, -16 },   "sc"    }, /* sc r7, r0, #-16 */
	{ 0xC0EF, 0x18, 0, { 0, 7, 15  },   "sc"    }, /* sc r0, r7, #15 */
	{ 0xC0F0, 0x18, 0, { 0, 7, -16 },   "sc"    }, /* sc r0, r7, #-16 */
	{ 0xC70F, 0x18, 0, { 7, 0, 15  },   "sc"    }, /* sc r7, r0, #15 */

	{ 0xC800, 0x19, 0, { 0, 0, 0  },    "mfcp"  }, /* mfcp r0, c0, x0 */
	{ 0xCF00, 0x19, 0, { 7, 0, 0  },    "mfcp"  }, /* mfcp r7, c0, x0 */
	{ 0xC8E0, 0x19, 0, { 0, 7, 0  },    "mfcp"  }, /* mfcp r0, c7, x0 */
	{ 0xC81F, 0x19, 0, { 0, 0, 31 },    "mfcp"  }, /* mfcp r0, c0, x31 */

	{ 0xD000, 0x1A, 0, { 0, 0, 0  },    "mtcp"  }, /* mtcp r0, c0, x0 */
	{ 0xD700, 0x1A, 0, { 7, 0, 0  },    "mtcp"  }, /* mtcp r7, c0, x0 */
	{ 0xD0E0, 0x1A, 0, { 0, 7, 0  },    "mtcp"  }, /* mtcp r0, c7, x0 */
	{ 0xD01F, 0x1A, 0, { 0, 0, 31 },    "mtcp"  }, /* mtcp r0, c0, x31 */

	{ 0xD800, 0x1B, 0, { 0, 0, 0 },     "cpop"  }, /* cpop r0, c0 */
	{ 0xDF00, 0x1B, 0, { 7, 0, 0 },     "cpop"  }, /* cpop r7, c0 */
	{ 0xD8E0, 0x1B, 0, { 0, 7, 0 },     "cpop"  }, /* cpop r0, c7 */

	{ 0xE000, 0x1C, 0, { 0,    0, 0 },  "int"   }, /* int #0 */
	{ 0xE7FF, 0x1C, 0, { 2047, 0, 0 },  "int"   }, /* int #2047 */

	{ 0xE800, 0x1D, 0, { 0, 0, 0 },     "rfi"   }, /* rfi */

	{ 0xF000, 0x1E, 0, { 0, 0,  0 },    "mfctl" }, /* mfctl r0, q0 */
	{ 0xF700, 0x1E, 0, { 7, 0,  0 },    "mfctl" }, /* mfctl r7, q0 */
	{ 0xF01F, 0x1E, 0, { 0, 31, 0 },    "mfctl" }, /* mfctl r0, q31 */

	{ 0xF800, 0x1F, 0, { 0, 0,  0 },    "mtctl" }, /* mtctl r0, q0 */
	{ 0xFF00, 0x1F, 0, { 7, 0,  0 },    "mtctl" }, /* mtctl r7, q0 */
	{ 0xF81F, 0x1F, 0, { 0, 31, 0 },    "mtctl" }, /* mtctl r0, q31 */
};

//#define VERBOSE

void run_case(int i)
{
	int opcode, subcode;
	uint16_t instr;
	char *nmemonic;
	int fields[3];

#ifdef VERBOSE
	printf("case %d: op=%x sub=%x fields=[%d %d %d]:\n",
	       i, cases[i].opcode, cases[i].subcode,
	       cases[i].fields[0], cases[i].fields[1], cases[i].fields[2]);
#endif

	assemble_instruction(&instr, cases[i].nmemonic, cases[i].fields);
	assert(instr == cases[i].instr);

	decode_instruction(&opcode, &subcode, &fields[0], cases[i].instr);
	assert(opcode == cases[i].opcode);
	assert(fields[0] == cases[i].fields[0]);
	assert(fields[1] == cases[i].fields[1]);
	assert(fields[2] == cases[i].fields[2]);
	assert(subcode == cases[i].subcode);

	resolve_nmemonic(&nmemonic, opcode, subcode);
	assert(! strcmp(nmemonic, cases[i].nmemonic));
}

int main(int argc, char *argv[])
{
	unsigned int i = 0;

	if(argc == 1) {
		for(i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
			run_case(i);
		}
	} else {
			run_case(strtol(argv[1], NULL, 0));
	}

	return 0;
}
