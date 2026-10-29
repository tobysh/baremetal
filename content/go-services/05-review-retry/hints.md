Structure it as `for i := 1; i <= attempts; i++ { ... }`. At the top of the loop: `if err := ctx.Err(); err != nil { return err }`.
===
For one attempt write a small helper: `actx, cancel := ctx, func(){}`; if `perAttempt > 0`, `actx, cancel = context.WithTimeout(ctx, perAttempt)`. Call `err := f(actx)` and then `cancel()` (not `defer`, it is in a loop).
===
After a failure, if `i == attempts` return `fmt.Errorf("after %d attempts: %w", i, err)`. Otherwise wait: `if backoff > 0 { t := time.NewTimer(backoff); select { case <-t.C: case <-ctx.Done(): t.Stop(); return ctx.Err() } }`.
