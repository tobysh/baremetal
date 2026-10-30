Implement `find_traversal(target, control, expected_control, marker, candidates, n)`.

Return `-2` if the control fetch does not return status 200 and an exact `expected_control` body. Otherwise fetch candidates unchanged in order and return the zero-based index of the first status-200 response with body exactly equal to `marker`. Return `-1` if none match. Stop immediately on control failure or proof; do not fetch later candidates. Always fetch the control, including when n is zero. NULL response bodies are failures. All input strings are non-NULL, n is 0–32, and the candidate array may be NULL only for n zero. Response bodies are NUL-terminated and valid until the next callback invocation. Do not mutate inputs.

The callback models a fictional endpoint; do not use file or network APIs. Hidden tests vary the marker, control result and successful candidate.
