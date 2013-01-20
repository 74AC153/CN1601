/*
int fact(int n) {
	int nextval, nextarg, ret;
	if(n == 0) {
		ret = 1;
		goto end;
	}

	if(n < 0) {
		ret = 0;
		goto end;
	}

	nextarg = n;
	--nextarg;
	nextval = fact(nextarg);
	ret = n * next;

end:
	return ret;
}

stack:

[ n           ] FP + 2
[ return addr ] FP + 1
[ return fp   ] <- FP
*/

#define FP r5
#define SP r6
#define LR r7

.data @fact;
.data @fact_base;
.data @fact_inval;
.data @fact_return;

.org 0xFF;
fact:
	deci SP, #2;   
	stw SP, LR, #0; /* push return addr */
	stw SP, FP, #0; /* push old frame pointer */
	or FP, SP, SP; /* set FP = SP */

	ldw r1, FP, #2; /* r1 = n */
	or r2, r1, r1; /* r2 = r1 = n */

	bz r1, %fact_base; /* if n == 0, goto fact_base */

	shri r2, #15; /* r2 = sign bit of n */
	bnz r2, %fact_inval; /* if n < 0, goto fact_inval */

	deci r1, #1; /* r1 = n - 1 */
	deci SP, #1;
	stw SP, r1, #0; /* push r1 */

	lui r2, @fact.hi;
	ori r2, @fact.lo;
	jlr r2; /* recursively call fact(n-1). result in r0 */

	ldw r1, FP, #2; /* r1 = n */
	mul r0, r0, r2; /* retval = n * fact(n-1) */
	ba %fact_return;

fact_base:
	xor r0, r0, r0; /* clear r0 */
	inci r0, #1;    /* ret value = 1 */
	ba %fact_return; /* return; */

fact_inval:
	xor r0, r0, r0; /* clear r0 */
	ba %fact_return;

fact_return:
	or SP, FP, FP; /* SP = FP */
	ldw FP, SP, #0; /* restore FP */
	ldw LR, SP, #1; /* restore LR */
	inci SP, #2; /* SP now points to input arg */
	jr r7; /* return */

