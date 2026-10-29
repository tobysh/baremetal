`str_length`: start `i` at 0 and add one while `s[i] != '\0'`. All three functions use that same loop condition, so you can write them as `for (int i = 0; s[i] != '\0'; i++)`.
===
For `count_vowels`, lower-case each character first (`tolower((unsigned char)s[i])`) so you only compare against five lowercase letters. For `to_upper`, a lowercase letter is one with `s[i] >= 'a' && s[i] <= 'z'`; subtract `'a'` and add `'A'` to move it, or call `toupper`.
===
```c
void to_upper(char s[]) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = (char)(s[i] - 'a' + 'A');
        }
    }
}
```
