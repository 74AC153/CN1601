	ori r0, #0;
	bal %forward;
	inci r0, #1;
	ba %end;
	inci r4, #1;
forward:
	inci r1, #1;
	jr r7, #0; 
	inci r2, #1;
end:
	lui r3, #0xFF;
	ori r3, #0xFF;
	jr r3, #0;
