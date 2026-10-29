#include <stdio.h>

typedef struct {
    char name[16];
    int score;
} Student;

int main(void) {
    Student students[20];
    int n = 0;
    if (scanf("%d", &n) != 1) n = 0;
    if (n > 20) n = 20;
    int count = 0;
    while (count < n && scanf("%15s %d", students[count].name, &students[count].score) == 2) {
        count++;
    }
    n = count;

    for (int i = 1; i < n; i++) {
        Student current = students[i];
        int j = i - 1;
        while (j >= 0 && students[j].score < current.score) {
            students[j + 1] = students[j];
            j--;
        }
        students[j + 1] = current;
    }

    int total = 0;
    int passed = 0;
    for (int i = 0; i < n; i++) {
        printf("%d. %s %d\n", i + 1, students[i].name, students[i].score);
        total += students[i].score;
        if (students[i].score >= 50) passed++;
    }
    double average = n > 0 ? (double)total / n : 0.0;
    printf("average: %.2f\n", average);
    printf("passed: %d of %d\n", passed, n);
    return 0;
}
