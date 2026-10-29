For `render_report`, first build a flattened copy of each value (replacing CR and LF with a space), then compute the full length with `snprintf(NULL, 0, ...)` or by adding the pieces, and only write if it fits.
===
For `lint_report`, walk the text line by line. Keep, for each of the five labels, the line number where it first appeared (or -1) and the value. Order is correct when the recorded positions of the present labels increase.
===
Bullets: after the EVIDENCE line, count lines that start with `- ` until you reach a line that starts with another label. For secrets, search each bullet for the three keys; the text right after the `=` must start with `[redacted]`.
