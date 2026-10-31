Slot write is `STOREX 1, r0, r1`, which stores `r1` at `m[1 + r0]`, with `r0` the tail you read and `r1` set to 1.
===
Flawed push: `LOAD r0,0`, `SETI r1,1`, `STOREX 1,r0,r1`, `ADDI r0,1`, `STORE 0,r0`, `END`.
===
Locked push: the same body between `LOCK 0` and `UNLOCK 0`.
===
Corrected push: `LOAD r0,0`, `MOV r2,r0`, `ADDI r2,1`, `CAS 0,r0,r2,r3`, `JZ r3, 0` (retry), then `SETI r1,1`, `STOREX 1,r0,r1`, `END`.
