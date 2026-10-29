Find the two spaces with `strchr`; the method is the text before the first, the target is between the two, the version is the rest.
===
Walk the target in segments: find the next `/`, compare a segment of length 2 with `..`, then move on. Only copy after every rule has passed.
