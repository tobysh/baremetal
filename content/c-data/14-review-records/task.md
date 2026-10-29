`Person` is defined in the starter; `name` holds up to 19 characters.

- `Person person_make(char name[], int age)` returns a person. Names longer than 19 characters are cut to their first 19, and `name` always ends in a terminator. Write the copy loop yourself.
- `int name_starts_with(Person p, char prefix[])` returns `1` if the name begins with `prefix` (case sensitive), otherwise `0`. An empty prefix matches every name.
- `void capitalise_names(Person people[], int n)` changes the first letter of each name to uppercase if it is a lowercase letter. Other characters are untouched.
- `int oldest_index(Person people[], int n)` returns the index of the person with the greatest age; the first one on a tie. `n` is at least 1.
