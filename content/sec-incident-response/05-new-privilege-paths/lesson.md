---
title: Trace new privilege paths
summary: Diff an identity graph before and after an incident, find the edges that created new routes to administrators, and attribute each to an audit record.
minutes: 40
difficulty: hard
tags: incident-response, identity, graphs
id: sec-incident-response/new-privilege-paths
mode: harness
---

Attackers who reach one account often add a path that keeps them privileged: a new account in a group that already leads to the administrators. In a directory graph this is an edge, and finding it means comparing the graph before and after rather than looking at one snapshot.

Three ideas matter. First, **who is newly privileged**: principals that can reach the target now but could not before. Second, **which added edges enabled that**: an added edge counts when its source is newly privileged and its destination can reach the target (or is the target). A redundant edge from someone who already had access is real but is not what changed the picture, and an edge into a group with no route to the target is noise. Third, **attribution**: the audit log should say who added each edge and when. An enabling edge with no audit record is itself a finding, since it means a gap in the log or a change made around it.

Only `MemberOf` and `HasRole` edges grant privilege here; other kinds (like `Manages`) are ignored. Graphs may contain cycles.

**Limitation:** an audit entry shows what the system recorded for an actor name. If that account was already stolen, the actor name points at the victim, not the intruder.
