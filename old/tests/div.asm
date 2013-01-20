lui r0, #0xFF;
inci r0, #0xF0;
inci r1, #0x0F;

div r2, r0, r0;
div r3, r1, r1;

div r4, r0, r1;
wshri r5, #0x10;

div r6, r1, r0;
wshri r7, #0x10;
