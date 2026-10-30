`Run(program []Instruction, caps map[string]string, maxOps int, maxTicks uint32) (Trace, error)` interprets the same bounded instruction list. `Assemble` is supplied as a validation helper.

- Validate the complete instruction list with the lesson 1 rules. maxOps must be positive and counts **all** instructions, including HALT. More than maxOps instructions is an error.
- EMIT appends its text to Outputs.
- READ accepts only a key present in caps; append the key to Reads and its supplied value to Outputs. A present key with an empty value is valid. Never consult host files or environment variables.
- WAIT adds its ticks to Trace.Ticks without sleeping. Reject a total above maxTicks.
- Total output bytes, including repeated READ values and EMIT text, must not exceed 4096.
- On success return nonnil Outputs and Reads slices, even when empty. On any failure return `Trace{}` and an error, with no partial results. Leave program and caps unchanged.

Keep the provided assembler and implement Run. A program `READ "note"; WAIT 2; EMIT "done"; HALT` with caps `{"note":"lab data"}`, maxOps 4 and maxTicks 2 returns Outputs `["lab data","done"]`, Reads `["note"]`, Ticks 2. Removing the note capability must reject the entire trace.

