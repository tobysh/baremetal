---
title: "Review: A Handler Test Kit"
summary: A consolidation exercise. Build small helpers for testing HTTP handlers from JSON, httptest, generics and testing.TB. No new ideas, just practice.
minutes: 30
difficulty: medium
kind: review
tags: testing, http, json
id: go-services/review-handler-test-kit
mode: harness
---
Every handler test repeats the same steps: encode a body, build a request, record the response, check the status, decode the JSON. In this review you write a four-function kit that removes that repetition. All the ingredients are old friends:

- building requests and recorders with `httptest` ([handlers](/learn/go-services/http-handlers)),
- JSON encoding and decoding ([JSON API](/learn/go-services/json-api)),
- helpers that take a `testing.TB`, call `t.Helper()` and choose between `Errorf` and `Fatalf` ([test helpers](/learn/go-services/test-helpers)).

The only twist is a generic function that returns a value of a type the caller chooses.
