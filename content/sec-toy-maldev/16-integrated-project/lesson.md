---
title: Capstone: configure, package, observe and tear down
summary: Build one end-to-end lab pipeline in checkpoints, from a configurable payload to a verified clean teardown, without breaking earlier stages.
minutes: 70
difficulty: hard
tags: security, toy-maldev, capstone, integration
id: sec-toy-maldev/integrated-project
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payloads emit text, read explicitly supplied map entries or advance virtual time. The "host" is a Go map in memory, the "deployment" two entries in it, and nothing ever runs outside the interpreter. Everything stays inside the existing Go sandbox.

This capstone joins the ideas of the whole course in one small lab pipeline:

1. **Configure and resolve.** A payload template refers to symbols such as `greeting`. A configuration map supplies them. Resolving substitutes the values, marks the entry point, and refuses typos in either direction (a symbol with no value, or a configuration value nobody uses).
2. **Package and load.** The resolved program is assembled, wrapped in a capsule whose entry is a *byte offset*, and packed in an envelope.
3. **Collect a trace.** The bundle is loaded and interpreted under capability and budget limits.
4. **Detect behavior.** The trace is compared with a rule set: reads of keys that were not expected, too much virtual time, forbidden output markers.
5. **Verify teardown.** The lab deploys the artifacts into the simulated host, and a journal ensures every entry created is removed again, even when a stage fails.

Work through the checkpoints in order. Each is tested on its own, and the tests for earlier checkpoints keep running against your final code: an optimization in stage 5 must not change what stage 1 returns. The helpers from earlier lessons (`Assemble`, `Wrap`, `Pack`, `Unpack`, `LoadCapsule`, `Run`, `ExecuteBundle`) are provided.

Two honest limits. The detector looks at the **executed trace only**; code before the entry point never runs, so the trace cannot show it, and the whole-image audit from the Loader Pipeline challenge answers that different question. And the teardown guarantee holds for the stated in-memory model, which has no crashes inside the teardown itself (the crash-recovery lesson covers interruption).

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
