Separate the browser sending a request from JavaScript reading the response.
===
Session lifecycle and cookie visibility are separate protections: rotate on login, invalidate on logout, and do not treat HttpOnly as an XSS fix.
