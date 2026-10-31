Implement `Attribute(logins []Login, reqs []Request, window int64) []Link`, returning one `Link{Index, User, Basis}` per request, in request order (`Index` is the request's position).

For a request:
1. If `Session` is non-empty: if some successful login (`OK`) has the same `Session` and `Time <= req.Time`, the link is that login's user with basis `"session"`. Otherwise basis `"unknown-session"` and empty user. Never fall back to the address in this case.
2. If `Session` is empty: collect distinct users with a successful login from the same `IP` with `req.Time-window <= Time <= req.Time`. One user: basis `"ip-time"`. Several: basis `"ambiguous"`, empty user. None: basis `"none"`, empty user.

Failed logins never count. Public example: user `kim` logs in from 192.0.2.5 at t=100 and gets session `s1`; a request at t=150 with session `s1` from any address links to `kim` by `"session"`.
