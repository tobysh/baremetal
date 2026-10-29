Read one line of text from standard input. It is English text that was encrypted with a Caesar shift between 1 and 25 (letters moved forward by the shift, case kept, everything else untouched). Print two lines:

```
shift: N
<the decrypted line>
```

where `N` is the shift that was used to encrypt, so the text is decrypted by shifting back by `N`. The tests feed in different messages and shifts.
