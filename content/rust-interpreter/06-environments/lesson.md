---
title: "Environments and Lexical Scope"
summary: Store variables in shared scope frames and resolve names through enclosing environments.
minutes: 45
difficulty: medium
tags: environments, rc, refcell, scope
id: rust-interpreter/environments
mode: harness
---
Builds on [Review: A Resilient Calculator](/learn/rust-interpreter/review-calculator).

Variables need storage that outlives a single recursive evaluator call. An **environment** associates names with values. A scope contains a local map and an optional enclosing environment; name lookup searches from the current scope outward.

`Env` wraps `Rc<RefCell<Frame>>`. `Rc` allows several handles to share one frame. Cloning an `Env` must preserve that identity rather than duplicate its map. `RefCell` permits mutation through a shared handle, checked at runtime. A child environment allocates a fresh frame whose parent is a clone of the enclosing handle.

`define` always writes to the current frame. It may shadow a parent binding without changing it. `get` searches here first, then recursively through parents. `assign` changes the nearest existing binding; it does not create a new one. If no frame contains the name, it returns false.

For example, a parent can define `x = 1`, a child can shadow it with `x = 2`, and assigning `x = 3` through that child changes only the child's binding. Assigning a different name that exists only in the parent reaches that parent. Another clone of either handle sees its mutations.

All environments belonging to one running program also share an output buffer. The provided `emit` and `take_output` methods use a second `Rc<RefCell<_>>`, so output from nested scopes stays ordered. Child frames must share this buffer, while two calls to `Env::new()` create independent programs.

The supplied evaluator now receives `&Env` and resolves `Expr::Var`. Keep `RefCell` borrows short and avoid holding a borrow while trying to mutate the same frame. These are single-threaded shared handles; concurrency needs different types.
