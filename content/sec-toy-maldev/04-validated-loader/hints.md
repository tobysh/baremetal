Decode boundaries into a byte-offset-to-instruction-index map. Never treat an arbitrary byte as an opcode merely because entry points there.
===
Compare every operand length against remaining bytes before slicing; validate the entire stream and final HALT, not only the portion after entry.
