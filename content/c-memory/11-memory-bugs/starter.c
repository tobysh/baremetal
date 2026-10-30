#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    int *scores;
    int count;
} Person;

Person *create_person(const char *name, int count) {
    Person *p = malloc(sizeof *p);
    p->name = malloc(strlen(name) + 1);
    strcpy(p->name, name);
    p->scores = calloc((size_t)count, sizeof(int));
    p->count = count;
    return p;
}

// Bug: leaks.
char *make_label(const char *name, int id) {
    char *tmp = malloc(strlen(name) + 16);
    sprintf(tmp, "%s#%d", name, id);
    char *result = malloc(strlen(tmp) + 1);
    strcpy(result, tmp);
    return result;
}

// Bug: uses the array after freeing it.
int sum_and_free(int *values, int n) {
    free(values);
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += values[i];
    }
    return total;
}

// Bug: frees one block twice and forgets another.
void release_person(Person *p) {
    free(p->name);
    free(p->name);
    free(p);
}

// Optional: try them with Run.
int main(void) {
    char *label = make_label("Ada", 7);
    printf("%s\n", label);
    free(label);

    int *v = malloc(3 * sizeof *v);
    v[0] = 1; v[1] = 2; v[2] = 3;
    printf("sum = %d (expected 6)\n", sum_and_free(v, 3));
    return 0;
}
