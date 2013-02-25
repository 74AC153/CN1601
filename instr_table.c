#include "instr_table.h"
/* types:
   - gp/ctlreg
   - branch
   - memory
   - coproc
   - mode
*/

/* for the assembler */
instr_spec_t instr_table [] = {
	{ "or",    0x00, 1,  0x0, "u3u3u3", "rrr"  },
	{ "nand",  0x00, 1,  0x1, "u3u3u3", "rrr"  },
	{ "and",   0x00, 1,  0x2, "u3u3u3", "rrr"  },
	{ "xor",   0x00, 1,  0x3, "u3u3u3", "rrr"  },
	{ "add",   0x01, 1,  0x0, "u3u3u3", "rrr"  },
	{ "sub",   0x01, 1,  0x1, "u3u3u3", "rrr"  },
	{ "addc",  0x01, 1,  0x2, "u3u3u3", "rrr"  },
	{ "subc",  0x01, 1,  0x3, "u3u3u3", "rrr"  },
	{ "shl",   0x02, 1,  0x0, "u3u3u3", "rrr"  },
	{ "shr",   0x02, 1,  0x1, "u3u3u3", "rrr"  },
	{ "shra",  0x02, 1,  0x2, "u3u3u3", "rrr"  },
	{ "rot",   0x02, 1,  0x3, "u3u3u3", "rrr"  },
	{ "shli",  0x03, 1,  0x0, "u3_2u4", "ru4"  },
	{ "shri",  0x03, 1,  0x1, "u3_2u4", "ru4"  },
	{ "shrai", 0x03, 1,  0x2, "u3_2u4", "ru4"  },
	{ "roti",  0x03, 1,  0x3, "u3_2u4", "ru4"  },
	{ "li",    0x04, 0,  0x0, "u3u8",   "ru8"  },
	{ "shlli", 0x05, 0,  0x0, "u3u8",   "ru8"  }, // redundant
	{ "inci",  0x06, 0,  0x0, "u3u8",   "ru8"  },
	{ "deci",  0x07, 0,  0x0, "u3u8",   "ru8"  },
	{ "ori",   0x08, 0,  0x0, "u3u8",   "ru8"  },
	{ "andi",  0x09, 0,  0x0, "u3u8",   "ru8"  },
	{ "xori",  0x0A, 0,  0x0, "u3u8",   "ru8"  },
	{ "orih",  0x0B, 0,  0x0, "u3u8",   "ru8"  },
	{ "andih", 0x0C, 0,  0x0, "u3u8",   "ru8"  },
	{ "xorih", 0x0D, 0,  0x0, "u3u8",   "ru8"  },
	{ "sleep", 0x0E, 0,  0x0, "_11",    ""     },
	{ "ba",    0x0F, 0,  0x0, "s11",    "s11"  },
	{ "bal",   0x10, 0,  0x0, "s11",    "s11"  },
	{ "bz",    0x11, 0,  0x0, "u3s8",   "rs8"  },
	{ "bnz",   0x12, 0,  0x0, "u3s8",   "rs8"  },
	{ "jr",    0x13, 0,  0x0, "u3s8",   "rs8"  },
	{ "jlr",   0x14, 0,  0x0, "u3s8",   "rs8"  },
	{ "ll",    0x15, 0,  0x0, "u3u3s5", "rrs5" },
	{ "ldw",   0x16, 0,  0x0, "u3u3s5", "rrs5" },
	{ "stw",   0x17, 0,  0x0, "u3u3s5", "rrs5" },
	{ "sc",    0x18, 0,  0x0, "u3u3s5", "rrs5" },
	{ "mfcp",  0x19, 0,  0x0, "u3u3u5", "rcx"  },
	{ "mtcp",  0x1A, 0,  0x0, "u3u3u5", "rcx"  },
	{ "cpop",  0x1B, 0,  0x0, "u3u3_5", "rc"   },
	{ "int",   0x1C, 0,  0x0, "u11",    "u11"  },
	{ "rfi",   0x1D, 0,  0x0, "_11",    ""     },
	{ "mfctl", 0x1E, 0,  0x0, "u3_3u5", "rq"   },
	{ "mtctl", 0x1F, 0,  0x0, "u3_3u5", "rq"   }
};

int instr_table_len = sizeof(instr_table) / sizeof(instr_table[0]);

