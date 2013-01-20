lui r0, #0xFF;
inci r0, #0xF0;
inci r1, #0x0F;

usub r2, r0, r0;
usub r3, r1, r1;

usub r4, r0, r1;
wshri r5, #0x10;

usub r6, r1, r0;
wshri r7, #0x10;
