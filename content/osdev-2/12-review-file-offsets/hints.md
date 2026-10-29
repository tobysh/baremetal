Keep each descriptor’s offset separately from the file bytes.
===
Limit the transfer to `32 - offset`, then advance the offset by the actual count.
