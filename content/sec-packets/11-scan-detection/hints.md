Store for each client a list of (server, port, outcome); before adding a target search the list, so the first outcome stays.
===
Qualify with `n>=5 && closed+filtered>open`, collect indexes into an array and insertion-sort by count descending, then `strcmp`.
