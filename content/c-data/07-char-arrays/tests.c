int main(void) {
    CHECK("str_length(\"hello\") is 5", str_length("hello") == 5, "got %d", str_length("hello"));
    CHECK("str_length(\"\") is 0", str_length("") == 0, "got %d", str_length(""));
    CHECK("str_length counts spaces and punctuation", str_length("a b, c!") == 7, "got %d", str_length("a b, c!"));
    char buf[10] = {'h', 'i', '\0', 'x', 'y', 'z', '\0'};
    CHECK("str_length stops at the first terminator", str_length(buf) == 2, "got %d", str_length(buf));

    CHECK("count_vowels(\"Hello, World\") is 3", count_vowels("Hello, World") == 3, "got %d", count_vowels("Hello, World"));
    CHECK("count_vowels counts upper case", count_vowels("AEIOU") == 5, "got %d", count_vowels("AEIOU"));
    CHECK("count_vowels(\"rhythm\") is 0", count_vowels("rhythm") == 0, "got %d", count_vowels("rhythm"));
    CHECK("count_vowels of empty is 0", count_vowels("") == 0, "got %d", count_vowels(""));
    CHECK("count_vowels counts repeats", count_vowels("banana") == 3, "got %d", count_vowels("banana"));
    CHECK("count_vowels ignores y", count_vowels("yyyy") == 0, "got %d", count_vowels("yyyy"));

    char a[] = "Hello, World 42!";
    to_upper(a);
    CHECK("to_upper changes letters only", strcmp(a, "HELLO, WORLD 42!") == 0, "got %s", a);
    char b[] = "ALREADY";
    to_upper(b);
    CHECK("to_upper leaves capitals alone", strcmp(b, "ALREADY") == 0, "got %s", b);
    char c[] = "";
    to_upper(c);
    CHECK("to_upper on empty text is safe", c[0] == '\0', "got %d", c[0]);
    char d[] = "az{`@[";
    to_upper(d);
    CHECK("to_upper does not touch characters next to the letter ranges", strcmp(d, "AZ{`@[") == 0, "got %s", d);
    char e[8] = {'a', 'b', '\0', 'c', 'd', '\0'};
    to_upper(e);
    CHECK("to_upper stops at the terminator", e[3] == 'c' && e[0] == 'A', "got %c %c", e[0], e[3]);
    return 0;
}
