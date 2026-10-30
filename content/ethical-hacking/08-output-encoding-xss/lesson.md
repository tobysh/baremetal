---
title: Output encoding for XSS
summary: Encode untrusted text for the HTML body and for a quoted attribute, allow only safe link schemes, and compose a link in a toy page renderer.
minutes: 35
difficulty: medium
tags: web, xss, output-encoding, html
id: ethical-hacking/output-encoding-xss
mode: harness
---
> **Simulated target.** Everything here is a string-handling exercise inside the sandbox. The "page" is text built by your functions and nothing is sent to a browser. The sample inputs are the standard harmless test strings that defenders use. Never test a technique from this course on a system you do not own or lack written permission to test; start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics).

**Cross-site scripting (XSS)** happens when text from a user is placed into a page without being made safe, and the browser treats it as markup or script instead of as data. A comment such as `<script>...</script>` then runs in every visitor's browser, with that site's cookies and permissions. The cure is not to try to *remove* bad input; it is to **encode on output**, for the exact place the data lands. The same string needs different treatment in different contexts.

- **HTML body** (`<p>HERE</p>`): replace the five characters that can start markup, `& < > " '`, with entities (`&amp; &lt; &gt; &quot; &#39;`). The `&` goes first in meaning: it must itself be encoded, or `&lt;` typed by a user would be decoded back into `<`.
- **Quoted attribute** (`<input value="HERE">`): the dangerous characters are the quote that ends the value, and in sloppy markup a space or `=` that starts a new attribute such as `onmouseover`. The safe rule is an allow-list: keep letters, digits and `. , - _`, and write every other byte as a numeric entity like `&#x22;`. Always quote the attribute.
- **URL** (`href`, `src`): encoding does not help if the URL itself is `javascript:...`. Allow-list the scheme (`http`, `https`, `mailto`) or require a relative reference. Control characters and whitespace inside the scheme are used to hide it, so reject them.

Other contexts (inside `<script>`, CSS, URLs as a whole) each have their own rules, which is why mature frameworks escape by default and why raw-HTML escape hatches are reviewed closely. Encoding protects the page; [cookie flags](/learn/sec-appsec/cookie-header-audit) limit the damage if something still gets through; and the same context-aware thinking applies to [SQL injection](/learn/ethical-hacking/sql-injection).
