---
title: "Capstone: The Stockroom"
summary: Extend a stockroom inventory with reservations, persist them in the stored format without stranding older readers, and upgrade old files while regression tests pin everything else.
minutes: 60
difficulty: hard
tags: capstone, migration, regression, compatibility
id: go-maintenance/capstone-stockroom
mode: harness
---
This capstone combines the whole course: read an unfamiliar module, change it without disturbing what works, extend a stored format with old and new data coexisting, and prove it against tests you cannot see.

## The situation

The stockroom module stores items as lines. Files in the field are in two shapes: `sku,qty` (the first release) and `sku,qty,price` (the current one). The current release reads both and writes the second shape. The new feature is **reservations**: stock can be set aside for an order, still counted as physically present but not available to others.

## Design constraints

- **Older readers keep working.** The previous release is still deployed on some machines and reads only 2 and 3 field lines. So the writer emits the **oldest format that can represent the data**: an item with no reservation is written as a 3-field line, exactly the bytes the current release would write; only an item with an outstanding reservation gets a 4th field. Machines that never see a reservation never see a format they cannot read.
- **Earlier behaviour is pinned.** `Low` and `Value` talk about *physical* stock and must keep doing so, even though "available" is tempting. Error text prefixes, ordering and parsing leniency are pinned too.
- **The upgrade tool keeps humans' work.** `Upgrade` rewrites a whole file in place into canonical form but leaves comments and blank lines alone.

## Honest limits

Distinguishing formats by field count works here because no format has optional middle fields; it would not survive adding a field in the middle. Hidden regression tests compare your module with an independently written reference over generated operation sequences. That is strong evidence, not proof, and it cannot check the operational side (staged rollout, backups) that a real migration needs.
