#include <stdio.h>

typedef struct {
    char name[20];
    int age;
} Person;

Person person_make(char name[], int age) {
    Person p = {"", 0};
    return p;
}

int name_starts_with(Person p, char prefix[]) {
    return -1;
}

void capitalise_names(Person people[], int n) {
}

int oldest_index(Person people[], int n) {
    return -1;
}

// Optional: try them with Run.
int main(void) {
    Person group[3];
    group[0] = person_make("ada", 36);
    group[1] = person_make("grace", 85);
    group[2] = person_make("linus", 54);
    capitalise_names(group, 3);
    printf("%s %s %s (expected Ada Grace Linus)\n", group[0].name, group[1].name, group[2].name);
    printf("oldest = %d (expected 1)\n", oldest_index(group, 3));
    printf("starts with Gr = %d (expected 1)\n", name_starts_with(group[1], "Gr"));
    return 0;
}
