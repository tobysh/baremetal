`RunJobs(ctx context.Context, limit int, jobs []func(ctx context.Context) (string, error)) ([]string, error)` runs the jobs concurrently, with at most `limit` running at any moment (treat `limit < 1` as `1`).

- Each job receives a context derived from `ctx`. That context is cancelled as soon as any job returns an error.
- The returned slice has one string per job, **in job order**. A job that failed or never ran leaves `""` in its slot.
- Before starting each job, check the context. If it is already done (a job failed, or the caller cancelled), do not start that job or any later one.
- Wait for all started jobs to finish before returning.
- Return the **first** error a job returned. If no job failed but `ctx` was cancelled before every job could start, return `ctx.Err()`. Otherwise return `nil`.
- With no jobs, return an empty non-nil slice and `nil`.
