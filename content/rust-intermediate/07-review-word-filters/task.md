Finish the word filtering code.

- `impl Matcher for Prefix<'_>`: true when the word starts with the stored prefix.
- `impl Matcher for MinLen`: true when the word has at least that many bytes.
- `fn select<'a, M: Matcher>(text: &'a str, m: &M) -> Vec<&'a str>`: the whitespace-separated words of `text` that match, in order.
- `fn select_all<'a>(text: &'a str, ms: &[&dyn Matcher]) -> Vec<&'a str>`: the words matching **every** matcher in the list. An empty list matches every word.
- `fn first_match<'a>(text: &'a str, m: &dyn Matcher) -> Option<&'a str>`: the first matching word, if any.

The results must not be tied to the matcher's lifetime, because the tests drop temporary matchers while still using the words.
