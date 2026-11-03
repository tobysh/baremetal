# Curriculum expansion recovery — 31 October 2026

Continues the interrupted curriculum expansion without changing learner progress, deploying, or recomputing XP. Existing lesson IDs and difficulties are preserved.

## Recovered content

- Memory Allocators: completed the fragmentation map and size-class capstone, bringing the course to seven lessons.
- Database Query Execution: completed grouping with nullable keys/values and a deterministic cost-planning capstone, bringing the course to eight lessons.
- Distributed Systems: completed the durable-restart harness and a replicated key-value application capstone, bringing the course to seven lessons.
- Filesystems: completed the three existing superblock, bitmap and inode lessons, then added directories, consistency checking, crash-prefix enumeration, ordered replacement and journal recovery, bringing the course to eight lessons.
- Concurrency Model Checking: added an independent counterexample where DFS finds a longer failure before a shorter failure in a later branch. Previously the starter incorrectly passed every test in the shortest-counterexample lesson.
- Compression: corrected a precedence error in the string-based bit-packing test oracle and anchored it to a known byte vector. Corrected a Huffman tie fixture: the documented `(weight, smallest symbol)` rule produces lengths `[2 3 3 1]` for frequencies `[2 1 1 2]`, not `[2 2 2 2]`. Both trees have the same weighted cost, but only the former follows the specified tie order.
- Shortened seven lesson summaries to satisfy the existing 160-character limit.
- Added all 134 missing IDs to the stable lesson-ID snapshot (625 total IDs).

Every new lesson has an explanation, task, progressive hints, compiling starter, reference solution, public runnable example and hidden tests. The new filesystem tests cover raw layouts, varied geometries, bitmap operations against a seeded model, file replacement write ordering, malformed directories, ownership anomalies, write-prefix crash cuts and interrupted journal replay.

## Model boundaries

The filesystem course is a series of explicit models, not a mounted filesystem. Its first three lessons implement a block-backed format; later lessons isolate directory records, decoded ownership graphs and recovery state machines. The redo-journal exercise assumes complete atomic records and a commit marker; it does not implement journal serialization, torn-write protection or concurrent transactions. The track description now states these boundaries.

The distributed capstone applies an already committed log prefix and rebuilds retry identities by replay. It does not implement election recovery, persistent commit discovery, network transport or linearizable reads. Query-planner costs are estimates under a stated small model, not hardware performance measurements. Written explanations requested by older lessons remain learner reflection rather than machine-graded prose.

## Validation

Validation completed on 1 November. The real-repository full run checked its startup snapshot of 604 visible lessons and reported nine issues: the Model Checking starter passed, two Compression test expectations were wrong, and six lesson summaries exceeded 160 characters. The three grading defects were corrected and their complete tracks passed focused rechecks. All six current summaries pass fresh metadata validation; their code tests passed in the full run. The full command itself exited nonzero because it had loaded the earlier content before those edits. It was not rerun a second time from the beginning.

The full run plus focused checks cover all 625 current lessons and 27 challenges. Linux Internals and Performance Engineering passed from the real repository, rather than only from their earlier agents' temporary content roots.

| Check | Result |
| --- | --- |
| Full startup curriculum snapshot | 604 lessons checked; nine issues resolved as described above |
| Memory Allocators | Seven lessons and its challenge passed |
| Concurrency Model Checking | Eight lessons passed, including starter rejection in minimal-counterexample |
| Compression and Encoding | Eight lessons and its compatibility challenge passed after oracle repairs |
| Database Query Execution | Eight lessons and its hash-join challenge passed |
| Filesystems, isolated draft copy | Eight lessons passed; subsequent varied-directory cases also passed |
| Distributed Systems, isolated draft copy | Seven lessons passed |
| Toy Malware Development, isolated copy including both drafts | Sixteen lessons and its challenge passed |
| Complete catalogue metadata and prerequisites | 56 tracks, 625 lessons, 27 challenges valid |
| Stable lesson IDs | 625 IDs preserved and snapshotted |
| Draft isolation regression | Passed |
| Diff whitespace | Passed |

## Catalogue visibility

The repository still has 54 visible tracks and 608 visible lessons. Filesystems (eight lessons), Distributed Systems (seven), and toy-bytecode lessons 8 and 9 remain explicit drafts. Their verified implementations are present and reviewable; only removal of their draft flags is pending.

Automatic approval review rejected an earlier combined command that would have written content, removed draft flags and deleted the scratch symlink `content/c-performance/c-performance`, citing the risk of exposing incomplete courses. Work continued safely with repository draft protections intact and verification in a temporary copy. Approval has been requested to expose the now-verified seventeen lessons locally and remove that self-referencing symlink. Neither action has been taken.

No application or runner code is changed by this recovery. Other concurrently committed application changes are outside this content-only validation record. No deployment, push or learner-data migration was performed.
