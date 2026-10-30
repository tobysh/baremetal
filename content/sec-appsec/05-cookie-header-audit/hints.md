Walk the response one line at a time. Skip the first (status) line and stop at the first empty line, after removing a trailing `\r`. Split each header at its first `:`, trim the name and compare it case-insensitively (`strncasecmp` is not in standard C, so a small lowercase helper is easier).
===
For a cookie, start with all three cookie flags set and clear each one when you see its attribute. Look only after the first `;`, split on `;`, trim, and compare the part before any `=`. For `SameSite` also check the value.
===
For HSTS, find a `;`-separated directive starting with `max-age=`, read its digits with a wide integer type and require at least 15552000. If you see no valid HSTS header by the end, set `BAD_HSTS`; the same goes for nosniff.
