Stdin: first line `idle absolute` (seconds), then at most 1000 events `<time> <action> <sid>` in time order, where action is `login`, `req` or `logout` and sid has at most 31 characters. Print `N: <result>` per event:

`login`: `ok` if the sid has never been used before (it creates a session with created = last = time), otherwise `rejected-reused-id`.
`req`: `unknown` if there is no live session; `expired-absolute` if `time - created > absolute` (destroy it); `expired-idle` if `time - last > idle` (destroy it); otherwise `ok` and last = time.
`logout`: `ok` if a live session exists (it is destroyed, whether or not it has expired by now), otherwise `unknown`.

A destroyed sid stays retired forever: it can never be logged in again.
