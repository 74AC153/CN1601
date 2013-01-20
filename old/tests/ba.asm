start:
	ba %forward;
	inci r0, #1;
back:
	inci r1, #1;
	ba %end;
	inci r2, #1;
forward:
	ba %back;
end:
	lui r3, #0xFF;
	ori r3, #0xFF;
	jr r3, #0;
