Implement `AdminPath(raw, start, goal string) ([]string, error)`. JSON is a Directory with Nodes and Edges as declared below. Find a shortest directed path using only MemberOf, GenericAll, AdminTo and HasSession. Sort neighbor IDs before expanding. Include start and goal in the returned path. Reject malformed JSON, duplicate/empty node IDs, unknown edge endpoints, or missing start/goal IDs (including edges of ignored kinds with invalid endpoints). Return an empty path and no error if unreachable; start==goal returns that one node. Labels and node types are metadata. `directory.json` is the public graph: expected route guest → helpdesk → workstation → admin → da.

Public fixture (`directory.json`):
```json
{
  "Nodes": [
    {
      "ID": "guest",
      "Type": "User",
      "Label": "Guest"
    },
    {
      "ID": "helpdesk",
      "Type": "Group",
      "Label": "Helpdesk"
    },
    {
      "ID": "workstation",
      "Type": "Computer",
      "Label": "WS-01"
    },
    {
      "ID": "admin",
      "Type": "User",
      "Label": "Admin"
    },
    {
      "ID": "da",
      "Type": "Group",
      "Label": "Domain Admins"
    }
  ],
  "Edges": [
    {
      "From": "guest",
      "To": "helpdesk",
      "Kind": "MemberOf"
    },
    {
      "From": "helpdesk",
      "To": "workstation",
      "Kind": "AdminTo"
    },
    {
      "From": "workstation",
      "To": "admin",
      "Kind": "HasSession"
    },
    {
      "From": "admin",
      "To": "da",
      "Kind": "MemberOf"
    }
  ]
}
```
