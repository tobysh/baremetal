int main(void) {
    CHECK("count_words: simple sentence", count_words("the quick brown fox") == 4, "got %d", count_words("the quick brown fox"));
    CHECK("count_words: extra spaces everywhere", count_words("  the quick  brown fox ") == 4, "got %d", count_words("  the quick  brown fox "));
    CHECK("count_words: one word", count_words("hello") == 1, "got %d", count_words("hello"));
    CHECK("count_words: empty text", count_words("") == 0, "got %d", count_words(""));
    CHECK("count_words: only spaces", count_words("     ") == 0, "got %d", count_words("     "));
    CHECK("count_words: punctuation belongs to the word", count_words("hi, there! ok?") == 3, "got %d", count_words("hi, there! ok?"));
    CHECK("count_words: single letters", count_words("a b c d e f") == 6, "got %d", count_words("a b c d e f"));

    CHECK("longest_word: quick is 5", longest_word("  the quick  brown fox ") == 5, "got %d", longest_word("  the quick  brown fox "));
    CHECK("longest_word: last word wins", longest_word("a bb cccc") == 4, "got %d", longest_word("a bb cccc"));
    CHECK("longest_word: first word wins", longest_word("elephant is big") == 8, "got %d", longest_word("elephant is big"));
    CHECK("longest_word: no words", longest_word("   ") == 0 && longest_word("") == 0, "expected 0");
    CHECK("longest_word: counts punctuation", longest_word("hi, there!") == 6, "got %d", longest_word("hi, there!"));

    char a[] = "Hello, Zoe!";
    caesar(a, 3);
    CHECK("caesar shifts by 3 with wraparound", strcmp(a, "Khoor, Crh!") == 0, "got %s", a);
    char b[] = "Khoor, Crh!";
    caesar(b, -3);
    CHECK("caesar with a negative shift undoes it", strcmp(b, "Hello, Zoe!") == 0, "got %s", b);
    char c[] = "abc xyz";
    caesar(c, 29);
    CHECK("caesar with shift 29 acts like 3", strcmp(c, "def abc") == 0, "got %s", c);
    char d[] = "Same 123";
    caesar(d, 0);
    CHECK("caesar with shift 0 changes nothing", strcmp(d, "Same 123") == 0, "got %s", d);
    char e[] = "abcz";
    caesar(e, -30);
    CHECK("caesar with a large negative shift", strcmp(e, "wxyv") == 0, "got %s", e);
    char f[] = "az AZ";
    caesar(f, 26);
    CHECK("caesar with shift 26 is the identity", strcmp(f, "az AZ") == 0, "got %s", f);
    char g[] = "{`@[";
    caesar(g, 5);
    CHECK("caesar leaves neighbours of the letter ranges alone", strcmp(g, "{`@[") == 0, "got %s", g);
    return 0;
}
