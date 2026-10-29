---
title: Contain a path under its root
summary: Normalise an untrusted URL-style path with a stack of segments, reject any attempt to climb above the root, and never overrun the output.
minutes: 40
difficulty: hard
tags: security, path-traversal, input-validation
id: sec-c-bugs/path-traversal
mode: harness
---
A file server turns the path in a request into a file under its document root. If the path contains `..` segments, a naive concatenation such as `root + path` lets a request climb out of the root and name files it was never meant to serve. This is path traversal. Rejecting paths that contain the text `..` is too crude (a file may legitimately be called `..a`), and checking the string before decoding or normalising it is a classic way to get bypassed. The robust approach is to normalise first, by resolving the segments the way a filesystem would, and then to verify that the result is still inside the root.

Treat the path as segments separated by `/`. An empty segment or `.` changes nothing. A `..` segment removes the previous segment, and if there is none the path is trying to escape, so reject it. Anything else is a name and is appended. Because the output is a fixed buffer, count room before every write, and leave space for the terminating zero. On any failure leave an empty string in the output so a careless caller cannot use half a path.

This function works on bytes only. Real systems must also think about percent-decoding order, symbolic links, case-insensitive filesystems and platform separators; opening the file with the operating system's own containment features is stronger than any string check. All paths here are made up.
