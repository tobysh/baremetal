Remember placeholder positions before pushing them. Patch with `code[index] = Op::JumpIfFalse(target)` or `Op::Jump(target)` after the destination is known.
===
A loop back edge targets the first condition instruction, not the body. Its false target is the instruction after the back edge. All indices include any prefix already in `code`.
