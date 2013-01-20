        li r0, #1
        bnz r0, %first
		xor r0, r0, r0
		bnz r0, %second
first:  li r0, #3
second: li r0, #4
        li r0, #0xFF
