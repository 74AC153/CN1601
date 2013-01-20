
; int fibo(int n)
; {
;     if(n <= 1) {
;         return 1;
;     }
;     return fibo(n-1) + fibo(n-2);
; }
;
; stack at entrypoint:
;
; 0xFFFF    arg1
; 0xFFFE -> prev FP <- SP, FP
;
; SP: r5
; FP: r6
; LR: r7
; V : r0

bootstrap:
	; init SP
	lui r5, #0xFF
	inci r5, #0xFF

	; load input and push as arg1
	lui r4, @input.hi
	ori r4, @input.lo
	ldw r4, r4, #0
	stw r5, r4, #0
	deci r5, #1

	; push prev FP
	lui r4, #0
	stw r5, r4, #0
	deci r5, #1

	; init FP
	or r6, r5, r5
	inci r6, #1
	
	; initial call
	lui r0, @fibo_start.hi
	ori r0, @fibo_start.lo
	jlr r0

	; halt
	lui r0, @halt.hi
	ori r0, @halt.lo
	jr r0
	

fibo_start:
	




halt:
	lui r4, #0xFF
	inci r4, #0xFE
	jr r4

input:
	.data #9
