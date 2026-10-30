Write one small helper that appends a character to the output while counting the total length: it stores the character only if there is room (leave one byte for the zero) and always increments the count. Every function can then be a loop over the input that calls the helper.
===
In `encode_html`, use a `switch` on the character and append the entity text for the five special cases. In `encode_attr`, test for allowed characters first; for everything else append `&#x`, the two hex digits from `"0123456789ABCDEF"`, and `;`. Cast to `unsigned char` before testing `>= 0x80`.
===
For `safe_url`, reject bad bytes with one loop, then reject a leading `//`. Scan for the first of `:`, `/`, `?`, `#`; if it is not a colon the URL is relative. Otherwise compare the text before the colon with the three schemes, ignoring case. `render_link` is the helpers glued together, so write the loops once and call them from both encoders and the link.
