`person_make` is the `item_make` copy loop with 19 instead of 15 as the bound: `while (name[i] != '\0' && i < 19)`. Then write `p.name[i] = '\0';` and the age. `oldest_index` is a maximum search that remembers the index, replacing it only on a strictly greater age.
===
`name_starts_with`: loop over `prefix` until its terminator, and return 0 as soon as `p.name[i] != prefix[i]`. If the name ends sooner than the prefix, the name's `'\0'` will not match a prefix letter, so the loop stops safely. `capitalise_names` only looks at `people[i].name[0]`.
===
```c
int name_starts_with(Person p, char prefix[]) {
    for (int i = 0; prefix[i] != '\0'; i++) {
        if (p.name[i] != prefix[i]) return 0;
    }
    return 1;
}
```
