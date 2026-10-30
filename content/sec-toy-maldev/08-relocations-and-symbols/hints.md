Write one helper that decodes a code slice into instruction start offsets and, separately, the CALL operand offsets. Both the export check and the relocation check become set lookups.
===
Compute each object's base before patching anything. Validate every object and the whole symbol table first, then build the output, so an error never returns partial bytes.
===
Copy each object's code into the output buffer and patch the copy. Use binary.LittleEndian.PutUint16 on the output at base+Site. Choose BML1 only if the totals show zero CALLs and exactly one HALT.
