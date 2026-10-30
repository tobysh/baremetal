Complete `BrowserAnswers() map[string]string`. Use these question IDs and exact answer labels:

| ID | Question | Labels |
|---|---|---|
| csrf-missing | State-changing request has session cookie but lacks required CSRF token. | reject / accept |
| cors-side-effect | A simple cross-origin form POST is sent but response is unreadable due to CORS. Can the server still change state? | possible / impossible |
| cors-wildcard | Credentialed fetch, wildcard Allow-Origin, Allow-Credentials true. May JavaScript read the response? | blocked / readable |
| cors-explicit | Credentialed fetch, matching explicit Allow-Origin, Allow-Credentials true. | blocked / readable |
| login-session | After successful login, what happens to the session identifier? | rotate / retain |
| httponly | Does HttpOnly alone prevent an injected script from making authenticated requests? | no / yes |
| logout | What must happen to the server-side session? | invalidate / retain |

This is a fixed conceptual quiz: fill the map directly. No parser or simulated browser is required.
