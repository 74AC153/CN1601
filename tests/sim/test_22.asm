        li r0, #1
        bz r0, %first
		xor r0, r0, r0
		bz r0, %second
first:  li r0, #3
second: li r0, #0xFF
        li r0, #6
