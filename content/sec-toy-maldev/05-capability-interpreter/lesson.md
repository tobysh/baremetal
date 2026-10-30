---
title: Interpret payloads with explicit capabilities
summary: Run the toy instruction set with permitted data, output limits and virtual-time budgets.
minutes: 40
difficulty: hard
tags: security, toy-maldev, capability-interpreter
id: sec-toy-maldev/capability-interpreter
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

An instruction can be structurally valid but unauthorized. READ is valid syntax for any nonempty key; whether that key may be read is determined by the caller's explicit capability map. A key that resembles a filesystem path is still only a map key. This interpreter never opens a host file.

Resource limits are also capabilities. The caller grants a finite instruction budget and a virtual-time budget, while the runtime enforces a fixed output bound. On rejection, return no partial trace. This models a fail-closed execution boundary with observable behavior.

`Run(program []Instruction, caps map[string]string, maxOps int, maxTicks uint32) (Trace, error)` interprets the same bounded instruction list. `Assemble` is supplied as a validation helper.

- Validate the complete instruction list with the lesson 1 rules. maxOps must be positive and counts **all** instructions, including HALT. More than maxOps instructions is an error.
- EMIT appends its text to Outputs.
- READ accepts only a key present in caps; append the key to Reads and its supplied value to Outputs. A present key with an empty value is valid. Never consult host files or environment variables.
- WAIT adds its ticks to Trace.Ticks without sleeping. Reject a total above maxTicks.
- Total output bytes, including repeated READ values and EMIT text, must not exceed 4096.
- On success return nonnil Outputs and Reads slices, even when empty. On any failure return `Trace{}` and an error, with no partial results. Leave program and caps unchanged.


Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
