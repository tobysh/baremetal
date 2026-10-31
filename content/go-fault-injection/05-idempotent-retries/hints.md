`Ledger` needs memory of past requests: `seen map[string]int` from id to amount.
===
Check `seen` before the balance check. A repeated id must succeed even if the balance is now too low to pay again.
===
`Pay`: loop `for attempt := 1; attempt <= maxAttempts; attempt++`. Return `nil` on success; `continue` only when `errors.Is(err, ErrTimeout)`; otherwise `return err`.
===
Never build a new id inside the loop. Wrap the final error with `fmt.Errorf("%w after %d attempts", ErrGaveUp, maxAttempts)`.
