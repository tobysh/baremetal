# Curriculum expansion plan

Status: proposed roadmap. This document gathers the course additions and exercise formats discussed in this session. It does not authorize deployment, infrastructure changes or implementation of every item.

## Starting point

Baremetal already has C, Rust, Go, assembly, OS development, database and security courses. Extend those foundations into projects that combine concepts and require investigation, maintenance and reasoning about failure.

The recently added security courses provide these starting points:

| Existing course | What it provides | Guide |
| --- | --- | --- |
| Security Investigation Labs | Capture analysis, subnetting, permissions, toy web targets, scan correlation and directory graphs. | [Security labs](security-labs.md) |
| Offensive Infrastructure Lab | Stateful toy targets, privilege escalation, account/service persistence and verified cleanup. | [Offensive infrastructure](offensive-infrastructure-lab.md) |
| Toy Malware Development | Inert bytecode, containers, packaging, loaders, a capability interpreter, task-message validation and an admission challenge. | [Toy malware development](toy-maldev.md) |

These are existing content, not new work proposed by this plan. New lessons should build on them rather than repeat their base exercises.

## Priorities and suggested order

| Stage | Focus | Reason to do it in this order |
| --- | --- | --- |
| 1 | Debugging and Profiling; Testing and Fault Injection | Both improve how learners approach every existing language and systems course. |
| 2 | Toy malware extensions: relocations, embedded configuration and behavior-based detection | Connect payload construction to the existing analysis course through a coherent project. |
| 3 | Linux Internals; Protocol Engineering; Software Maintenance | Turn isolated functions into systems with state, compatibility requirements and explicit contracts. |
| 4 | Concurrency Model Checking; Performance Engineering | Develop investigation skills around execution order, measurements and resource behavior. |
| 5 | Larger systems projects selected from the backlog below | Choose based on prerequisite coverage, learner feedback and available simulation support. |

Exercise-format improvements can be introduced in every stage. These stages are a suggested sequence, not delivery dates or effort estimates. Start with one pilot course rather than implementing the whole backlog at once.

## First pilot: Debugging and Profiling

Teach learners to diagnose unfamiliar programs using evidence before changing code.

- Diagnose a reproducible crash and identify its cause.
- Find memory corruption using guard regions or supplied diagnostic output.
- Reconstruct a deadlock from a thread-state snapshot.
- Locate a slow path using measurements rather than intuition.
- Compare a proposed fix with a baseline and guard against regressions.
- Finish with a mixed investigation requiring a diagnosis, patch and explanation.

Use deterministic failing programs and supplied traces first. Check debugger/profiler availability inside the runner before depending on interactive tools. Distinguish functional correctness from performance claims, and avoid unstable timing thresholds as the sole hidden-test oracle.

## Second pilot: Testing and Fault Injection

Teach correctness when operations fail partway through.

- Handle short and truncated writes.
- Inject allocation failures at different operation boundaries.
- Interrupt storage updates and recover a consistent state.
- Model clocks moving backward or advancing unexpectedly.
- Retry operations without duplicating their effects.
- Finish by testing an existing storage engine under a deterministic failure schedule.

Begin with explicit fake clocks, allocators and storage callbacks. Hidden tests vary the failure position and assert invariants such as preserved committed data, bounded retries and successful recovery.

## Toy malware-development extensions

All additions retain the current inert bytecode and simulated-data model. Extend the format deliberately, with versioning and compatibility tests where its schema changes.

| Proposed addition | Learner task | Learning outcome and validation |
| --- | --- | --- |
| Relocations and symbol resolution | Resolve named functions and patch offsets inside an invented bytecode image. | Understand loader linking; test out-of-range relocations, unresolved symbols, duplicates and correct patched offsets. |
| Embedded configuration | Build and parse versioned configuration records with optional fields. | Understand format evolution and malformed input; test truncation, unknown fields, defaults and round trips. Explain why encoding does not protect secrets. |
| Virtual scheduling | Schedule simulated tasks with delays, retries, cancellation and a fixed virtual clock. | Understand lifecycle and timing; verify deterministic ordering, bounded retries and cancellation without background processes or network traffic. |
| Behavioral equivalence | Produce differently packaged artifacts that yield identical execution traces. | Distinguish representation from behavior; compare traces across varied inputs rather than relying on byte signatures. |
| Signed artifact manifests | Verify a signed manifest before admitting a payload. | Extend the existing trusted-digest exercise into authenticity and trust management; test changed manifests, wrong keys and invalid signatures. Use established cryptographic primitives. |
| Crash recovery and teardown | Interrupt a simulated run, restart it and remove every recorded state change. | Practice idempotency and partial-failure recovery; inject interruption at each lifecycle step and audit the final state. |
| Behavior-based detection | Write detectors against interpreter traces and evaluate benign counterexamples. | Connect construction with analysis; test meaningful behaviors, misleading signatures and false positives. |
| Adversarial testing | Generate malformed capsules and minimize inputs that break a toy loader. | Practice fuzzing, input reduction and reproducible bug reports; keep seeds and minimized counterexamples stable. |

Recommended first additions: relocations, embedded configuration and behavior-based detection. Follow with scheduling and lifecycle recovery, then artifact signing and broader adversarial testing.

### Integrated final project

Build a configurable payload, resolve its symbols, package and load it, collect its trace, detect its behavior and verify complete teardown. Extend the project incrementally rather than presenting one large unfamiliar specification.

Acceptance checks should cover format validity, authorized behavior, detector results, failure recovery and absence of residual simulated state. Preserve the distinction between a valid artifact, an authentic artifact and permitted behavior.

## General course backlog

These courses deepen connections between existing tracks.

| Proposed course | Example project | Prerequisite connection and purpose |
| --- | --- | --- |
| Linux Internals | Build miniature file-descriptor tables, permission checks, signals and process trees. | Connect C, shell programming, OS development and security. |
| Compilers and Executable Formats | Write a linker, resolve symbols, apply relocations and inspect a small executable. | Bridge the interpreter, assembly and kernel tracks. Establish format-reading skills before native loader work. |
| Distributed Systems | Implement replication, leader election, retries and recovery in a simulated cluster. | Extend Go concurrency and database work using deterministic message delivery, partitions and restarts. |
| Protocol Engineering | Build a framed protocol with negotiation, checksums, backpressure and malformed-message handling. | Combine networking, parsing and defensive programming. Test arbitrary chunk boundaries and partial I/O. |
| Reverse Engineering | Infer an unknown binary format or reconstruct a function from assembly. | Complement assembly and malware analysis with behavioral inference and evidence-backed hypotheses. |
| Incident Response | Investigate a fictional compromise through captures, logs, filesystem state and identity graphs. | Combine the security courses into one evidence-driven investigation with a timeline and justified findings. |
| Secure Software Delivery | Verify artifacts, inspect dependency manifests, manage synthetic secrets and test update rollback. | Connect development with deployment and supply-chain security through explicit trust boundaries. |
| Performance Engineering | Compare data layouts, allocation strategies, cache behavior and algorithms. | Explain practical consequences of low-level choices. Separate reproducible measurements from unsupported claims. |

Debugging and Profiling and Testing and Fault Injection are part of this general backlog, detailed above as the recommended first pilots.

## Additional systems directions

These broaden the curriculum into substantial projects. Scope each against existing lessons to avoid duplicating introductory material.

| Proposed course | Example project | Core concepts |
| --- | --- | --- |
| Embedded Systems | Write a driver against simulated registers, interrupts and a fake sensor. | Register contracts, device state machines, interrupt handling and bounded polling. |
| Filesystems | Build a tiny filesystem with directories, allocation, consistency checks and crash recovery. | Persistent metadata, ownership of blocks, invariants and repair. |
| Memory Allocators | Implement an arena, a free-list allocator and a fragmentation visualizer. | Alignment, reuse, coalescing, allocation policies and memory overhead. |
| Concurrency Model Checking | Explore thread schedules and find one that causes a race or deadlock. | Interleavings, state exploration and minimal counterexample schedules. |
| Database Query Execution | Build table scans, joins, indexes and a small query planner. | Query semantics, execution strategies and cost tradeoffs; extend the storage-focused database work. |
| Compression and Encoding | Implement run-length encoding, Huffman coding and a streaming decoder. | Bit-level representations, streaming state, malformed inputs and expansion limits. |
| Language Interoperability | Call C from Rust or Go while managing ownership, layout and errors. | ABI boundaries, lifetimes and resource ownership. Confirm FFI toolchain support before making this a runner-backed course. |
| Software Maintenance | Extend an unfamiliar codebase while preserving compatibility and migrating stored data. | Reading existing code, local changes, migration safety and regression testing. |

Concurrency Model Checking is a distinctive candidate for an interactive experience: step through a simulated scheduler and observe exactly how a seemingly correct program fails. Begin with deterministic schedule exploration before investing in scheduler UI.

## Exercise formats across all tracks

Introduce these alongside new courses and selectively retrofit existing tracks. Explanations and review tasks may need assessment mechanisms beyond executable hidden tests; do not pretend output checks alone measure the quality of a justification.

| Format | Learner activity | Assessment approach |
| --- | --- | --- |
| Debugging exercise | Fix an existing implementation with limited clues. | Reproduce the defect, then verify the fix against varied regression inputs. |
| Cross-track project | Reuse earlier work in a larger system. | Test component contracts and integrated behavior, preserving earlier invariants. |
| Code review | Identify defects and justify proposed changes. | Use an authored defect inventory and explanation rubric; add executable counterexamples where appropriate. |
| Transfer test | Apply a familiar concept in a different setting. | Vary the surface problem while testing the same underlying invariant. |
| Project checkpoints | Build one substantial artifact over several lessons. | Require each checkpoint to preserve the behavior established earlier. |
| Mystery program | Infer behavior from inputs and outputs before seeing the source. | Test predictions on withheld examples and distinguish plausible hypotheses with counterexamples. |
| Patch challenge | Make a small, justified fix. | Prioritize correctness and preserved behavior; use patch size as a constraint only when it improves the learning objective. |
| Failure investigation | Reconstruct a failure from incomplete evidence. | Require a supported causal account and separate observations from assumptions. |
| Design comparison | Implement two approaches, measure them and explain tradeoffs. | Check equivalent results, meaningful measurement methodology and evidence supporting the comparison. |
| Compatibility challenge | Support an old format while introducing a new one. | Test mixed versions, defaults, migrations and rejection of ambiguous input. |
| Teach-back checkpoint | Explain an invariant or failure mode, then confront a counterexample. | Use explicit criteria and counterexamples to assess whether the explanation generalizes. |

## Cross-track project candidates

| Project | Courses combined | Final artifact |
| --- | --- | --- |
| Recoverable storage service | Database, protocols, concurrency and fault injection | A framed service that preserves committed state through simulated failures. |
| Inspectable execution pipeline | Interpreter, assembly, executable formats and debugging | A small program translated, linked and executed with inspectable intermediate artifacts. |
| Simulated device and filesystem | Embedded systems, memory allocation and filesystems | A driver-backed storage model with consistency checks and recovery. |
| End-to-end security investigation | Networking, Linux internals, infrastructure labs and incident response | An evidence package, reconstructed timeline, confirmed findings and verified remediation. |
| Configurable inert payload lab | Toy malware development, reverse engineering and detection | A packaged artifact, behavior trace, detector and teardown audit. |

## Implementation and validation rules

- Reuse the established lesson layout: explanation, task, starter, reference solution, progressive hints and hidden tests. Give each lesson a concrete learning outcome and observable completion condition.
- Preserve published lesson IDs. Add new IDs to the snapshot and check prerequisite relationships.
- Use the existing optional challenge mechanism for harder final exercises. A challenge should require additional reasoning and must not already pass with the base lesson solution.
- Vary hidden inputs and failure schedules. Prefer independent expected results over checking only agreement between two learner-written functions.
- Use deterministic simulations for clocks, devices, network delivery, scheduling and infrastructure state. Investigate unsupported toolchain capabilities before promising an exercise that requires them.
- Keep security targets fictional and their permitted effects explicit. New teaching content should not require expanding sandbox privileges or deploying public vulnerable services.
- Include public examples that learners can actually run. Check packaged fixtures against the reference implementation.
- Run the affected content verifier and lesson-ID checks. For prerequisite changes, run learning checks. Run code checks appropriate to any application or runner changes.
- Record limitations honestly: a simulation demonstrates its stated model, and a timing check does not by itself establish a general performance claim.

## Next implementation steps

- [x] Select Debugging and Profiling as the first pilot, or explicitly choose another item.
- [ ] Inventory existing debugging and failure-handling lessons to avoid repetition.
- [ ] Write the pilot's outcomes, prerequisites, lesson sequence and final challenge contract.
- [x] Implement one complete lesson, including public example and independent hidden tests (`c-debugging/guard-bytes`).
- [x] Verify its starter, solution and failure feedback before expanding the course (`npm run verify:content -- c-debugging`).
- [ ] Complete the pilot and gather learner feedback before choosing the next stage.
- [ ] Extend Toy Malware Development with relocations, embedded configuration and behavior-based detection.
- [ ] Introduce selected debugging, compatibility and transfer exercises into existing tracks.

All proposed additions remain pending until implemented and verified. This plan makes no delivery-date commitments.
