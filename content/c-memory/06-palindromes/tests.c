int main(void) {
    CHECK("is_palindrome(\"\") is 1", is_palindrome("") == 1, "got %d", is_palindrome(""));
    CHECK("is_palindrome(\"a\") is 1", is_palindrome("a") == 1, "got %d", is_palindrome("a"));
    CHECK("is_palindrome(\"aa\") is 1", is_palindrome("aa") == 1, "got %d", is_palindrome("aa"));
    CHECK("is_palindrome(\"ab\") is 0", is_palindrome("ab") == 0, "got %d", is_palindrome("ab"));
    CHECK("is_palindrome(\"racecar\") is 1 (odd length)", is_palindrome("racecar") == 1, "got %d", is_palindrome("racecar"));
    CHECK("is_palindrome(\"noon\") is 1 (even length)", is_palindrome("noon") == 1, "got %d", is_palindrome("noon"));
    CHECK("is_palindrome(\"hello\") is 0", is_palindrome("hello") == 0, "got %d", is_palindrome("hello"));
    CHECK("is_palindrome(\"abca\") is 0", is_palindrome("abca") == 0, "got %d", is_palindrome("abca"));
    CHECK("is_palindrome(\"abcdba\") is 0 (mismatch deep inside)", is_palindrome("abcdba") == 0, "got %d", is_palindrome("abcdba"));
    CHECK("is_palindrome(\"Level\") is 0 (case-sensitive)", is_palindrome("Level") == 0, "got %d", is_palindrome("Level"));
    CHECK("is_palindrome(\"a b a\") is 1 (spaces count)", is_palindrome("a b a") == 1, "got %d", is_palindrome("a b a"));
    CHECK("is_palindrome(\"ab a\") is 0 (spaces count)", is_palindrome("ab a") == 0, "got %d", is_palindrome("ab a"));

    CHECK("is_palindrome_ignoring_case(\"\") is 1", is_palindrome_ignoring_case("") == 1, "got %d", is_palindrome_ignoring_case(""));
    CHECK("is_palindrome_ignoring_case(\"Z\") is 1", is_palindrome_ignoring_case("Z") == 1, "got %d", is_palindrome_ignoring_case("Z"));
    CHECK("is_palindrome_ignoring_case(\"Level\") is 1", is_palindrome_ignoring_case("Level") == 1, "got %d", is_palindrome_ignoring_case("Level"));
    CHECK("is_palindrome_ignoring_case(\"RaceCar\") is 1", is_palindrome_ignoring_case("RaceCar") == 1, "got %d", is_palindrome_ignoring_case("RaceCar"));
    CHECK("is_palindrome_ignoring_case(\"Noon\") is 1 (even length)", is_palindrome_ignoring_case("Noon") == 1, "got %d", is_palindrome_ignoring_case("Noon"));
    CHECK("is_palindrome_ignoring_case(\"Hello\") is 0", is_palindrome_ignoring_case("Hello") == 0, "got %d", is_palindrome_ignoring_case("Hello"));
    CHECK("is_palindrome_ignoring_case(\"Ab a\") is 0 (spaces count)", is_palindrome_ignoring_case("Ab a") == 0, "got %d", is_palindrome_ignoring_case("Ab a"));

    char high[] = {(char)0xE9, 'a', (char)0xE9, '\0'};
    CHECK("is_palindrome_ignoring_case handles bytes above 127 safely", is_palindrome_ignoring_case(high) == 1, "got %d", is_palindrome_ignoring_case(high));
    return 0;
}
