Build a tiny request-context API. Use your own unexported key types, so that other code using plain strings as keys can never collide with yours.

- `WithRequestID(ctx context.Context, id string) context.Context` and `RequestID(ctx context.Context) (string, bool)`. The bool is `false` when no ID was set.
- `WithUser(ctx context.Context, name string) context.Context` and `User(ctx context.Context) string`, which returns `"anonymous"` when no user was set. The two values are independent, and a later `WithUser` replaces an earlier one.
- `Describe(ctx context.Context) string` returns `"req=<id> user=<name>"`, using `-` for a missing request ID (so `req=- user=anonymous` for an empty context).
- `Detach(ctx context.Context) context.Context` returns a context that still carries `ctx`'s values but is never cancelled and has no deadline.
