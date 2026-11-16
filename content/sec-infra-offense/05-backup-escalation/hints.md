Keep the exact original config before editing; the proof must come from /root/proof after the job runs.
===
After an accepted write, restore the config and drop privilege even if the proof fails. Verify fresh state rather than relying on cleanup response codes.
===
Marshal the original `BackupConfig`, change only `Command`, and keep the original value to compare with later. `/backup/run` returning 202 is not proof of anything.
===
Structure it as: validate, write, run, read proof, then cleanup in a block that always executes once the write was accepted. Fold the proof and the clean-state check into the final return.
