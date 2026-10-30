int main(void) {
    CHECK("count_char(\"hello\", 'l') is 2", count_char("hello", 'l') == 2, "got %zu", count_char("hello", 'l'));
    CHECK("count_char(\"hello\", 'z') is 0", count_char("hello", 'z') == 0, "got %zu", count_char("hello", 'z'));
    CHECK("count_char(\"\", 'a') is 0", count_char("", 'a') == 0, "got %zu", count_char("", 'a'));
    CHECK("count_char(\"aaaa\", 'a') is 4", count_char("aaaa", 'a') == 4, "got %zu", count_char("aaaa", 'a'));
    CHECK("count_char(\"abc\", '\\0') is 0", count_char("abc", '\0') == 0, "got %zu", count_char("abc", '\0'));

    char a[] = "abcdef";
    reverse_prefix(a, 3);
    CHECK("reverse_prefix(\"abcdef\", 3) is cbadef", strcmp(a, "cbadef") == 0, "got %s", a);
    char b[] = "abcdef";
    reverse_prefix(b, 6);
    CHECK("reverse_prefix(\"abcdef\", 6) is fedcba", strcmp(b, "fedcba") == 0, "got %s", b);
    char c[] = "abcd";
    reverse_prefix(c, 0);
    reverse_prefix(c, 1);
    CHECK("reverse_prefix with n of 0 or 1 changes nothing", strcmp(c, "abcd") == 0, "got %s", c);
    char d[] = "abcde";
    reverse_prefix(d, 4);
    CHECK("reverse_prefix(\"abcde\", 4) leaves the tail alone", strcmp(d, "dcbae") == 0, "got %s", d);
    char e[] = "";
    reverse_prefix(e, 0);
    CHECK("reverse_prefix on the empty string is safe", e[0] == '\0', "got %d", e[0]);

    CHECK("phrase: Panama", is_phrase_palindrome("A man, a plan, a canal: Panama") == 1, "got %d", is_phrase_palindrome("A man, a plan, a canal: Panama"));
    CHECK("phrase: racecar", is_phrase_palindrome("racecar") == 1, "got %d", is_phrase_palindrome("racecar"));
    CHECK("phrase: Was it a car", is_phrase_palindrome("Was it a car or a cat I saw?") == 1, "got %d", is_phrase_palindrome("Was it a car or a cat I saw?"));
    CHECK("phrase: hello is 0", is_phrase_palindrome("hello") == 0, "got %d", is_phrase_palindrome("hello"));
    CHECK("phrase: mismatch hidden by punctuation", is_phrase_palindrome("ab, ca") == 0, "got %d", is_phrase_palindrome("ab, ca"));
    CHECK("phrase: empty is 1", is_phrase_palindrome("") == 1, "got %d", is_phrase_palindrome(""));
    CHECK("phrase: only punctuation is 1", is_phrase_palindrome("?! ,.") == 1, "got %d", is_phrase_palindrome("?! ,."));
    CHECK("phrase: digits count", is_phrase_palindrome("12 3-21") == 1, "got %d", is_phrase_palindrome("12 3-21"));
    CHECK("phrase: digit mismatch", is_phrase_palindrome("12 4-31") == 0, "got %d", is_phrase_palindrome("12 4-31"));
    char high[] = {(char)0xE9, '!', (char)0xE9, '\0'};
    CHECK("phrase: bytes above 127 are handled safely (not letters in the C locale)", is_phrase_palindrome(high) == 1, "got %d", is_phrase_palindrome(high));
    return 0;
}
