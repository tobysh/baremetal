Keep a map from credential name to earliest compromise time and another for hosts.
===
Loop over all events repeatedly, applying any whose actor is compromised by then, until a full pass changes nothing.
===
Only lower a time when the new one is strictly smaller; that makes the loop finish.
