Run the program: the AoS step prints 128 misses. Which bytes of each 64-byte line does the loop actually use?
===
The starter's `advance_x_soa` still reads `ps`. Switch it to the two arrays `sx` and `svx` that `soa_load` filled.
===
`st(&sx[i], ld(&sx[i]) + ld(&svx[i]))` for each `i`. That is three accesses per particle and 8 lines per array.
