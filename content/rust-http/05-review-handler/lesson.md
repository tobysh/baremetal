---
title: "Review: Handling a Request"
summary: A consolidation exercise. Join parsing, routing and response building into one handler, and validate a Content-Length header. No new ideas, just practice.
minutes: 25
difficulty: medium
kind: review
tags: http, parsing, routing, review
id: rust-http/review-handler
mode: harness
---
This is a **review lesson**. It introduces nothing new. So far you have written three separate pieces of an HTTP server: something that **parses** the raw text of a request, something that **builds** the text of a response, and a **router** that decides what to say. Before the course puts them behind a real network socket, this exercise connects them with a single function.

## What you already have

The starter contains your finished code from the earlier lessons, ready to call:

- `parse_request(raw) -> Result<Request, String>` turns text into a `Request` with `method`, `path`, `headers` (names lowercased) and `body`.
- `build_response(status, content_type, body) -> String` produces the exact bytes of a response.
- `route(method, path) -> (u16, String)` picks a status and a body.

Skim them first. If any of it looks unfamiliar, that is a sign worth going back to the lesson it came from.

## The handler

`handle(raw)` is the function a server would call once it has read a request off the wire. Its job is to turn request text into response text, and to **never panic** whatever the client sends. A server that crashes on bad input is a server that anyone can switch off. So every failure becomes a proper `400` response instead.

The steps, in order:

1. **Parse.** If `parse_request` returns `Err`, respond `400` with the body `bad request`.
2. **Check the body length.** A request may carry a `Content-Length` header that says how many bytes of body to expect. If the header is present, it has to be a valid number that equals the **byte** length of the body (`str::len`, not `chars().count()`). Otherwise the client has lied or been cut off, and you respond `400` with `bad request`. With no such header, there is nothing to check. That test belongs in its own helper, `body_matches_length`, because it is easy to get subtly wrong and easy to test on its own.
3. **One special route.** A `POST` to `/shout` answers `200` with the request body in upper case. Anything with a body is possible here, including none.
4. **Everything else** goes to `route`, and whatever status and body it returns are put through `build_response`.

All responses use the content type `text/plain`.

## Details to be careful with

- Header **names** were lowercased by the parser, so look up `"content-length"`, not `"Content-Length"`.
- `"abc"`, `"-1"` and `""` are not valid lengths. Parsing into a `usize` rejects all three for free.
- Because the special route comes *after* the length check, a lying `POST /shout` is still a `400`.
- `route` already decides what to do with unknown paths and wrong methods. Don't repeat that logic.

The result is a complete request-to-response function with no networking in it. That is precisely what makes it easy to test, and it is the design the next lesson relies on.
