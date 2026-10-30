---
title: Capstone: package, load and trace a payload
summary: Connect the package decoder, validating loader and bounded interpreter into one fail-closed pipeline.
minutes: 40
difficulty: hard
tags: security, toy-maldev, loader-pipeline
id: sec-toy-maldev/loader-pipeline
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

A loader chain has several independent boundaries. Packaging validates its envelope, the capsule loader validates structure and entry boundaries, and the interpreter enforces capabilities and budgets. A checksum-valid package can still contain an invalid capsule; a valid capsule can still request unauthorized data.

This capstone supplies working implementations from previous lessons. Your job is the composition. Decode, load, then interpret the suffix beginning at Image.Entry. Every stage must finish successfully before the next stage's result can be accepted. The returned trace is the observable behavior of the toy payload.

Instructions before entry are structurally validated by the loader but not executed. That distinction matters for the final challenge: a policy may want to audit the entire artifact, including code that is not reached in this run.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
