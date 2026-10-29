Build it in layers and run the tests often: first the authorization and range checks, then the port loop with retries, then banners and findings, then the document test, then the stable sort by severity.
===
For the sort, insertion sort is stable and plenty for 16 entries. Do not compare anything but severity, since discovery order within a severity must stay as found.
===
`report_text`: work out each block with `snprintf` into a large local buffer, flatten line breaks in the values first, and only copy to `out` if `length + 1 <= cap`.
