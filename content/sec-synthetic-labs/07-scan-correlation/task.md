Implement `Correlate(scan string, table []Advisory) []Exposure`. Match only valid open TCP ports (1..65535), with exactly four columns and exact Service and Version. Return unique Port/ID pairs sorted by port then ID. Different advisories may match one port. Public scan:
```
PORT STATE SERVICE VERSION
22/tcp open labssh 1.0
443/tcp open labhttp 3.2
9000/tcp filtered labhttp 1.0
```
Public table: `[{Service:"labssh", Version:"1.0", ID:"LAB-001"}]`; expected `[{22,"LAB-001"}]`. The same data ships as `scan.txt` and `advisories.json`.

Public fixture (`advisories.json`):
```json
[
  {
    "Service": "labssh",
    "Version": "1.0",
    "ID": "LAB-001"
  }
]
```
