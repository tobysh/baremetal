#include <stdio.h>

typedef struct {
    char name[20];
    int age;
} Person;

Person person_make(char name[], int age) {
    Person p;
    int i = 0;
    while (name[i] != '\0' && i < 19) {
        p.name[i] = name[i];
        i++;
    }
    p.name[i] = '\0';
    p.age = age;
    return p;
}

int name_starts_with(Person p, char prefix[]) {
    for (int i = 0; prefix[i] != '\0'; i++) {
        if (p.name[i] != prefix[i]) return 0;
    }
    return 1;
}

void capitalise_names(Person people[], int n) {
    for (int i = 0; i < n; i++) {
        char c = people[i].name[0];
        if (c >= 'a' && c <= 'z') {
            people[i].name[0] = (char)(c - 'a' + 'A');
        }
    }
}

int oldest_index(Person people[], int n) {
    int best = 0;
    for (int i = 1; i < n; i++) {
        if (people[i].age > people[best].age) best = i;
    }
    return best;
}

int main(void) {
    Person group[3];
    group[0] = person_make("ada", 36);
    group[1] = person_make("grace", 85);
    group[2] = person_make("linus", 54);
    capitalise_names(group, 3);
    printf("%s %s %s\n", group[0].name, group[1].name, group[2].name);
    printf("oldest = %d\n", oldest_index(group, 3));
    printf("starts with Gr = %d\n", name_starts_with(group[1], "Gr"));
    return 0;
}
