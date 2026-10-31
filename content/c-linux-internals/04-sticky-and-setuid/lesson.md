---
title: Sticky Directories and setuid
summary: Removal depends on the directory, not the file. The sticky bit narrows it, and setuid or setgid programs run with another user's identity.
minutes: 30
difficulty: medium
tags: permissions, sticky-bit, setuid
id: c-linux-internals/sticky-and-setuid
mode: harness
---
`may_access` from the last lesson is given. Three more rules make up most of what administrators need to know.

## Deleting is a directory operation

A directory is a list of names. To remove a name you change the list, so you need **write and search (x) permission on the directory**. The file's own mode and owner do not matter: a mode-`000` file can be removed, and a file you own cannot be removed from a read-only directory.

## The sticky bit

A world-writable directory such as `/tmp` would let anyone delete anyone's files. With the **sticky bit** (`01000`) set on the directory, removal additionally requires that the caller is **root, the file's owner, or the directory's owner**.

## setuid and setgid

If a program file has the setuid bit (`04000`), running it sets the process's **effective** uid to the file's owner; the real uid is unchanged. The setgid bit (`02000`) does the same for the effective gid. This is how a program can let ordinary users do one narrow privileged job, and why such programs are a classic source of vulnerabilities: they run with the owner's rights on input the caller controls.

The caller must pass the **execute check first**, using the credentials as they were before the exec.

## Model limits

The helper program in the example is fictional and nothing here executes anything: `do_exec` only edits a `cred` structure. Real Linux ignores setuid on shell scripts, on `nosuid` mounts and under `no_new_privs`, resets the saved uid, and handles capabilities; none of that is modelled.

## Your turn

Implement `may_unlink` and `do_exec`. Hidden tests sweep every directory mode and every program mode against a separate reference.
