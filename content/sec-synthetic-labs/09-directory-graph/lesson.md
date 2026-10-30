---
title: Find a lateral movement route in a directory graph
summary: Search directed membership, ACL and session edges for a shortest route to a privileged group.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, directory-graph
id: sec-synthetic-labs/directory-graph
mode: harness
---

Directory attack analysis is graph analysis. A user can belong to a group, a group can administer a computer, and that computer can hold a session for another identity. A path combines those relationships into a possible route to higher privilege. It is evidence for review, not proof that exploitation is possible under every configuration.

This lab uses a small normalized BloodHound-inspired JSON graph, not the native BloodHound import schema. Node IDs are opaque; names such as Domain Admins are display labels, not identifiers. Edges point in the direction control could advance: MemberOf user→group, GenericAll principal→object, AdminTo principal→computer, and HasSession computer→user. A session edge here explicitly assumes the lab's credential-access precondition. Ignore other edge kinds. Real environments need additional prerequisites, tiering, mitigations and edge-specific semantics.

Breadth-first search finds the route with the fewest edges. Track visited nodes to stop cycles. For reproducibility, visit outgoing neighbor IDs in lexical order. Do not infer reverse edges. A missing route should return an empty path, not an invented connection.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
