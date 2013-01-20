or r0, r0, r0
or r7, r0, r0
or r0, r7, r0
or r0, r0, r7
nand r0, r0, r0
nand r7, r0, r0
nand r0, r7, r0
nand r0, r0, r7
and r0, r0, r0
and r7, r0, r0
and r0, r7, r0
and r0, r0, r7
xor r0, r0, r0
xor r7, r0, r0
xor r0, r7, r0
xor r0, r0, r7
add r0, r0, r0
add r7, r0, r0
add r0, r7, r0
add r0, r0, r7
sub r0, r0, r0
sub r7, r0, r0
sub r0, r7, r0
sub r0, r0, r7
addc r0, r0, r0
addc r7, r0, r0
addc r0, r7, r0
addc r0, r0, r7
subc r0, r0, r0
subc r7, r0, r0
subc r0, r7, r0
subc r0, r0, r7
shl r0, r0, r0
shl r7, r0, r0
shl r0, r7, r0
shl r0, r0, r7
shr r0, r0, r0
shr r7, r0, r0
shr r0, r7, r0
shr r0, r0, r7
shra r0, r0, r0
shra r7, r0, r0
shra r0, r7, r0
shra r0, r0, r7
shli r0, #0
shli r7, #0
shli r0, #15
shli r7, #15
shri r0, #0 
shri r7, #0 
shri r0, #15
shri r7, #15
shrai r0, #0 
shrai r7, #0 
shrai r0, #15 
shrai r7, #15 
li r0, #0
li r7, #0
li r0, #255
li r7, #255
shlli r0, #0
shlli r7, #0
shlli r0, #255
shlli r7, #255
inci r0, #0
inci r7, #0
inci r0, #255
inci r7, #255
deci r0, #0
deci r7, #0
deci r0, #255
deci r7, #255
ori r0, #0
ori r7, #0
ori r0, #255
ori r7, #255
andi r0, #0
andi r7, #0
andi r0, #255
andi r7, #255
xori r0, #0
xori r7, #0
xori r0, #255
xori r7, #255
lhswp r0 
lhswp r7
ba #0
ba #-1024
ba #1023
bal #0
bal #0
bal #0
bz r0, #0
bz r0, #-128
bz r0, #127
bz r7, #-128
bz r7, #127
bnz r0, #0
bnz r0, #-128
bnz r0, #127
bnz r7, #-128
bnz r7, #127
jr r0, #0
jr r7, #0
jr r0, #-128
jr r0, #127
jr r7, #-128
jr r7, #127
jlr r0, #0
jlr r7, #0
jlr r0, #-128
jlr r0, #127
jlr r7, #-128
jlr r7, #127
ll r0, r0, #0
ll r7, r0, #-16
ll r0, r7, #15
ll r0, r7, #-16
ll r7, r0, #15
ldw r0, r0, #0
ldw r7, r0, #-16
ldw r0, r7, #15
ldw r0, r7, #-16
ldw r7, r0, #15
stw r0, r0, #0
stw r7, r0, #-16
stw r0, r7, #15
stw r0, r7, #-16
stw r7, r0, #15
sc r0, r0, #0
sc r7, r0, #-16
sc r0, r7, #15
sc r0, r7, #-16
sc r7, r0, #15
mfcp r0, c0, x0
mfcp r7, c0, x0
mfcp r0, c7, x0
mfcp r0, c0, x31
mtcp r0, c0, x0
mtcp r7, c0, x0
mtcp r0, c7, x0
mtcp r0, c0, x31
cpop r0, c0
cpop r7, c0
cpop r0, c7
int #0
int #2047
rfi
mfctl r0, q0
mfctl r7, q0
mfctl r0, q31
mtctl r0, q0
mtctl r7, q0
mtctl r0, q31
