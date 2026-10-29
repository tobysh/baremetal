int main(void) {
    Person a = person_make("ada", 36);
    CHECK("person_make stores name and age", strcmp(a.name, "ada") == 0 && a.age == 36, "got \"%s\", %d", a.name, a.age);
    Person l = person_make("abcdefghijklmnopqrstuvwxyz", 1);
    CHECK("person_make cuts names to 19 characters", strcmp(l.name, "abcdefghijklmnopqrs") == 0, "got \"%s\"", l.name);
    Person x = person_make("1234567890123456789", 2);
    CHECK("person_make keeps exactly 19 characters", strcmp(x.name, "1234567890123456789") == 0, "got \"%s\"", x.name);
    Person e = person_make("", 0);
    CHECK("person_make accepts an empty name", e.name[0] == '\0' && e.age == 0, "got %d", e.name[0]);

    Person g = person_make("Grace", 85);
    CHECK("name_starts_with a real prefix", name_starts_with(g, "Gr") == 1, "expected 1");
    CHECK("name_starts_with the whole name", name_starts_with(g, "Grace") == 1, "expected 1");
    CHECK("name_starts_with is case sensitive", name_starts_with(g, "gr") == 0, "expected 0");
    CHECK("name_starts_with a longer prefix is 0", name_starts_with(g, "Graceful") == 0, "expected 0");
    CHECK("name_starts_with a non-prefix is 0", name_starts_with(g, "ra") == 0, "expected 0");
    CHECK("name_starts_with an empty prefix is 1", name_starts_with(g, "") == 1, "expected 1");
    CHECK("name_starts_with on an empty name", name_starts_with(e, "a") == 0 && name_starts_with(e, "") == 1, "expected 0 and 1");

    Person grp[4];
    grp[0] = person_make("ada", 36);
    grp[1] = person_make("Grace", 85);
    grp[2] = person_make("linus", 54);
    grp[3] = person_make("9lives", 5);
    capitalise_names(grp, 4);
    CHECK("capitalise_names fixes lower-case first letters", strcmp(grp[0].name, "Ada") == 0 && strcmp(grp[2].name, "Linus") == 0, "got %s and %s", grp[0].name, grp[2].name);
    CHECK("capitalise_names leaves capitals alone", strcmp(grp[1].name, "Grace") == 0, "got %s", grp[1].name);
    CHECK("capitalise_names leaves digits alone", strcmp(grp[3].name, "9lives") == 0, "got %s", grp[3].name);
    CHECK("capitalise_names keeps ages", grp[0].age == 36 && grp[1].age == 85 && grp[2].age == 54, "ages changed");
    Person two[2];
    two[0] = person_make("bo", 1);
    two[1] = person_make("cy", 2);
    capitalise_names(two, 1);
    CHECK("capitalise_names respects n", strcmp(two[0].name, "Bo") == 0 && strcmp(two[1].name, "cy") == 0, "got %s and %s", two[0].name, two[1].name);
    capitalise_names(two, 0);
    CHECK("capitalise_names with n = 0 changes nothing", strcmp(two[1].name, "cy") == 0, "table changed");

    CHECK("oldest_index finds the oldest", oldest_index(grp, 4) == 1, "got %d", oldest_index(grp, 4));
    Person t[3];
    t[0] = person_make("a", 40);
    t[1] = person_make("b", 40);
    t[2] = person_make("c", 39);
    CHECK("oldest_index prefers the first on a tie", oldest_index(t, 3) == 0, "got %d", oldest_index(t, 3));
    CHECK("oldest_index with one person", oldest_index(t, 1) == 0, "got %d", oldest_index(t, 1));
    t[2].age = 99;
    CHECK("oldest_index with the oldest last", oldest_index(t, 3) == 2, "got %d", oldest_index(t, 3));
    return 0;
}
