#include <stdio.h>

int main(void) {
    int day;
    if (scanf("%d", &day) != 1) return 1;
    switch (day) {
        case 1: printf("Monday\n"); break;
        case 2: printf("Tuesday\n"); break;
        case 3: printf("Wednesday\n"); break;
        case 4: printf("Thursday\n"); break;
        case 5: printf("Friday\n"); break;
        case 6: printf("Saturday\n"); break;
        case 7: printf("Sunday\n"); break;
        default:
            printf("invalid day\n");
            return 0;
    }
    switch (day) {
        case 1: case 2: case 3: case 4: case 5:
            printf("weekday\n");
            break;
        default:
            printf("weekend\n");
            break;
    }
    return 0;
}
