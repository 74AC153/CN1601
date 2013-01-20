inci r0, #2     ;
sc   r1, r0, #0 ; overwrite the following instruction ...
inci r1, #0xF0  ; ... but this shouldn't get executed because we didn't ll
ll   r2, r0, #0 ; Now do the ll. r2 will hold value of opcode at offset 0
lui  r0, #0     ;
inci r0, #7     ;
sc   r1, r0, #0 ; this should work...
inci r3, #0x0F  ; ... this instruction shouldn't get executed
