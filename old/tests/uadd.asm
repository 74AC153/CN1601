lui r0, #0xFF;
inci r0, #0xF0;
inci r1, #0x0F;

uadd r2, r0, r0;
wshri r3, #0x10;

uadd r4, r0, r1;
wshri r5, #0x10;

uadd r6, r1, r1;
wshri r7, #0x10;
