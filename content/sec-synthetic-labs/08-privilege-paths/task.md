Implement `EscalationPaths(raw string) ([]string, error)` for the JSON State structure in the starter. Return sorted unique paths for writable files executed as root by a cron entry or eligible sudo rule. The user `root` has no escalation to report. Ignore non-root jobs, password-required sudo rules, rules for other users and paths absent from Files. Reject malformed JSON. Example: guest in ops can modify mode 0770 root:ops `/opt/backup.sh`; a root cron entry for that path makes it a finding. Public `state.json` uses this example. This is analysis only; never execute a command named by the input.

Public fixture (`state.json`):
```json
{
  "User": "guest",
  "Groups": [
    "ops"
  ],
  "Files": [
    {
      "Path": "/opt/backup.sh",
      "Mode": 504,
      "Owner": "root",
      "Group": "ops"
    }
  ],
  "Cron": [
    {
      "RunAs": "root",
      "Path": "/opt/backup.sh"
    }
  ],
  "Sudo": []
}
```
