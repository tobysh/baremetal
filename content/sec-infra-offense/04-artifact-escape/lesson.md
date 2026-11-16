---
title: Read beyond a toy artifact root
summary: Craft a path traversal request and distinguish a protected-file marker from a reflected filename.
minutes: 40
difficulty: hard
tags: security, offensive-lab, artifact-escape
id: sec-infra-offense/artifact-escape
mode: harness
---

> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the only 'host' is a `Lab` object the tests pass to your code, and `Lab.Send` is a plain Go callback, not a socket. In scope: the one port and the two paths the exercise hands you. The "filesystem" is a lookup in test memory; no real file is opened. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

A download service serves files from one directory, its **artifact root**, say `/srv/artifacts`. Its job is to refuse anything outside it. The vulnerable version checks the rule on the text the client sent, and only afterwards resolves `..` segments. The order of those two steps is the whole bug.

## The model

```text
requested:   /srv/artifacts/../../private/key
check:       starts with "/srv/artifacts/" ?   yes -> allowed
resolve:     /srv/artifacts/../../private/key  ->  /private/key
served:      the file at /private/key           (outside the root!)

patched:     resolve first -> /private/key -> does it start with "/srv/artifacts/"? no -> 403
```

`..` means "go up one directory". Climbing from `/srv/artifacts` takes two steps to reach `/`, because the root has two components. After reaching `/` you append the protected path.

## A worked example

Root `/lab/releases/public` (three components), protected file `/private/seed1.key`:

```text
root              = /lab/releases/public         components: lab, releases, public -> depth 3
payload           = /lab/releases/public/ + ../../../ + private/seed1.key
                  = /lab/releases/public/../../../private/seed1.key
raw prefix check  : starts with /lab/releases/public/    pass
after cleaning    : /private/seed1.key                    the protected file
```

In Go, the depth is the number of path elements:

```go
depth := len(strings.Split(strings.Trim(root, "/"), "/"))
payload := root + "/" + strings.Repeat("../", depth) + strings.TrimPrefix(protected, "/")
```

## Pitfalls

- Cleaning the path on your side with `path.Clean`. That removes the very prefix the vulnerable check relies on, and the server then refuses it.
- Miscounting components, so you climb out of the root only part-way.
- Treating any 200 as a win. A service may echo the path back (`requested path: ...`); only a body starting `LAB-PROOF:` shows the protected file was read.
- Sending several guesses. The lab permits exactly one request.

## The defender's half

Normalize first, then check the boundary on the result, as the patched model does. Better: resolve the path against the root with a function that refuses to leave it, and open the file relative to a directory handle so the OS enforces the boundary. Real filesystems add symbolic links and races between check and use, which this toy model leaves out; they are the reason string checks alone are never enough.
