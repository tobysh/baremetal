int main(void) {
    char empty[] = "";
    reverse(empty);
    CHECK("reverse(\"\") stays empty", empty[0] == '\0', "first byte was %d", empty[0]);

    char one[] = "a";
    reverse(one);
    CHECK("reverse(\"a\") stays \"a\"", strcmp(one, "a") == 0, "got \"%s\"", one);

    char two[] = "ab";
    reverse(two);
    CHECK("reverse(\"ab\") gives \"ba\"", strcmp(two, "ba") == 0, "got \"%s\"", two);

    char odd[] = "hello";
    reverse(odd);
    CHECK("reverse(\"hello\") gives \"olleh\" (odd length)", strcmp(odd, "olleh") == 0, "got \"%s\"", odd);

    char even[] = "abcd";
    reverse(even);
    CHECK("reverse(\"abcd\") gives \"dcba\" (even length)", strcmp(even, "dcba") == 0, "got \"%s\"", even);

    char sentence[] = "Hello, World!";
    reverse(sentence);
    CHECK("reverse(\"Hello, World!\") gives \"!dlroW ,olleH\"", strcmp(sentence, "!dlroW ,olleH") == 0, "got \"%s\"", sentence);

    char pal[] = "racecar";
    reverse(pal);
    CHECK("reverse(\"racecar\") is unchanged", strcmp(pal, "racecar") == 0, "got \"%s\"", pal);

    char twice[] = "reverse me";
    reverse(twice);
    reverse(twice);
    CHECK("reversing twice restores \"reverse me\"", strcmp(twice, "reverse me") == 0, "got \"%s\"", twice);

    char guard[] = "abc\0XYZ";
    reverse(guard);
    CHECK("reverse does not touch bytes after the terminator", guard[3] == '\0' && guard[4] == 'X' && guard[5] == 'Y' && guard[6] == 'Z', "bytes after terminator changed");
    CHECK("reverse(\"abc\") gives \"cba\" with trailing data present", strcmp(guard, "cba") == 0, "got \"%s\"", guard);
    return 0;
}
