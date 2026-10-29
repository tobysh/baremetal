---
title: "Review: audit a host's exposure"
summary: Combine scanning with retries, an allow-list audit, banner parsing and advisory matching into one exposure report for a simulated host.
minutes: 30
difficulty: medium
tags: review, recon, audit
id: ethical-hacking/review-exposure-audit
mode: harness
kind: review
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the host, its ports, banners and advisories are all produced by the test harness. In scope: the two callback functions it passes you. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

This review puts together ideas you already have: the [port scanner](/learn/ethical-hacking/port-scanner) with its retries, the allow-list audit, and the [banner and advisory](/learn/ethical-hacking/banner-identification) check. Nothing new is introduced. The only twist is that it all has to end up in one list of findings that someone could act on.

For every open port, in ascending order, produce these findings in this order:

1. `F_UNEXPECTED` if the port is not on the allow-list.
2. If the port has a banner of the form `product/version`:
   - `F_OUTDATED` if an advisory for that product has a `fixed_in` newer than the version (first matching advisory wins, and `adv` records its index);
   - nothing if it is current.
   If the banner has no `/` it is only a product name: that is `F_UNVERIFIED` when an advisory exists for the product, because a hidden version cannot be cleared.
   A port with no banner (`NULL`) gets no banner finding.

A defender who runs this regularly sees new exposure the day it appears instead of the day somebody else finds it.
