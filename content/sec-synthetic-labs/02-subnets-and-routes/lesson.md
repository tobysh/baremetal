---
title: IPv4 subnetting and route selection
summary: Compute IPv4 ranges and usable hosts, then choose a route using longest-prefix matching.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, subnets-and-routes
id: sec-synthetic-labs/subnets-and-routes
mode: harness
---

A CIDR prefix fixes the high bits of an address. Clearing the remaining bits yields the network address; setting them yields the broadcast address. For ordinary subnets, network and broadcast are reserved. In this lab /31 is a point-to-point link with two usable addresses, and /32 has one usable address. Report the upper address in the Broadcast field even for these special prefixes.

Routers select the matching route with the longest prefix. They do not select the first match or the numerically smallest gateway. For equal-length matches, keep the first route in the input. An unmatched destination has no next hop.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
