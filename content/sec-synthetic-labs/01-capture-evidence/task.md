Implement `AnalyzeCapture(raw string) (Evidence, error)`. JSON is an array of Packet objects. Return the first qualifying transfer's destination port, or zero if none, plus sorted unique nonempty DNS queries (an empty slice is acceptable). Reject malformed JSON. The public capture below is also shipped as `capture.json`:
```json
[{"proto":"DNS","query":"backup.lab.test"},{"proto":"TCP","src":"a","dst":"b","sport":41000,"dport":8443,"flags":"S"},{"proto":"TCP","src":"b","dst":"a","sport":8443,"dport":41000,"flags":"SA"},{"proto":"TCP","src":"a","dst":"b","sport":41000,"dport":8443,"flags":"A"},{"proto":"TCP","src":"a","dst":"b","sport":41000,"dport":8443,"flags":"PA","payload":"POST /store HTTP/1.1\r\nHost: backup.lab.test\r\nX-Lab-Transfer: archive\r\n\r\n"}]
```
Expected port: 8443; queries: `["backup.lab.test"]`.
