int main(void) {
    CHECK("str_equal: same text", str_equal("hello", "hello") == 1, "expected 1");
    CHECK("str_equal: different last letter", str_equal("hello", "hellp") == 0, "expected 0");
    CHECK("str_equal: prefix is not equal", str_equal("hell", "hello") == 0 && str_equal("hello", "hell") == 0, "expected 0");
    CHECK("str_equal: two empty strings", str_equal("", "") == 1, "expected 1");
    CHECK("str_equal: empty vs text", str_equal("", "a") == 0 && str_equal("a", "") == 0, "expected 0");
    CHECK("str_equal is case sensitive", str_equal("Cat", "cat") == 0, "expected 0");
    char x[8] = {'a', 'b', '\0', 'q', 'q', '\0'};
    char y[8] = {'a', 'b', '\0', 'z', 'z', '\0'};
    CHECK("str_equal ignores everything after the terminator", str_equal(x, y) == 1, "expected 1");

    CHECK("str_compare: equal strings give 0", str_compare("apple", "apple") == 0, "got %d", str_compare("apple", "apple"));
    CHECK("str_compare: apple before banana", str_compare("apple", "banana") < 0, "got %d", str_compare("apple", "banana"));
    CHECK("str_compare: banana after apple", str_compare("banana", "apple") > 0, "got %d", str_compare("banana", "apple"));
    CHECK("str_compare: a prefix comes first", str_compare("car", "cart") < 0, "got %d", str_compare("car", "cart"));
    CHECK("str_compare: a longer string comes after its prefix", str_compare("cart", "car") > 0, "got %d", str_compare("cart", "car"));
    CHECK("str_compare: empty comes first", str_compare("", "a") < 0 && str_compare("a", "") > 0, "got %d", str_compare("", "a"));
    CHECK("str_compare: capitals sort before lower case", str_compare("Zed", "apple") < 0, "got %d", str_compare("Zed", "apple"));
    char hi[2] = {(char)0xE9, '\0'};
    CHECK("str_compare treats bytes as unsigned", str_compare(hi, "z") > 0, "got %d", str_compare(hi, "z"));

    char d[16];
    str_copy(d, "hello");
    CHECK("str_copy copies the text", strcmp(d, "hello") == 0, "got %s", d);
    char e[16] = "XXXXXXXXXXXXXXX";
    str_copy(e, "ab");
    CHECK("str_copy writes the terminator", e[2] == '\0' && e[0] == 'a' && e[1] == 'b', "got %d", e[2]);
    str_copy(e, "");
    CHECK("str_copy of empty text gives empty text", e[0] == '\0', "got %d", e[0]);

    char f[32] = "tea";
    str_append(f, "spoon");
    CHECK("str_append joins the text", strcmp(f, "teaspoon") == 0, "got %s", f);
    char g[32] = "";
    str_append(g, "abc");
    CHECK("str_append onto empty text", strcmp(g, "abc") == 0, "got %s", g);
    str_append(g, "");
    CHECK("str_append of empty text changes nothing", strcmp(g, "abc") == 0, "got %s", g);
    str_append(g, "-");
    str_append(g, "xyz");
    CHECK("str_append can be repeated", strcmp(g, "abc-xyz") == 0, "got %s", g);
    return 0;
}
