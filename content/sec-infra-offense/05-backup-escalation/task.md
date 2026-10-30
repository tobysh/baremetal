Implement `ExerciseBackup(lab Lab, port int, token string) (Finding, bool)`. Use token on every request.

1. GET `/identity` must return 200 Body `user`.
2. GET `/backup/config` returns 200 JSON BackupConfig. Continue only for Owner `root`, Writable true, and Command `archive`. Path is metadata.
3. PUT `/backup/config` with the same JSON fields but Command `grant-root`. Proceed only on 200.
4. POST `/backup/run`, then GET `/root/proof`. A 200 proof Body beginning `LAB-PROOF:` confirms elevated access; run status alone does not.
5. After any accepted configuration write, always attempt cleanup: PUT the original config, POST `/session/drop-root`, then GET `/backup/config` and GET `/identity`. Cleanup succeeds only on 200 fresh reads showing the original config exactly and role `user`.

Return Finding,true only if impact and cleanup both succeed; otherwise zero Finding,false. No other paths/commands are allowed. Even if exploitation fails after the write, run cleanup. The model has no concurrent writers.
