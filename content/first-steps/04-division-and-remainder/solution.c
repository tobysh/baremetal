#include <stdio.h>

int main(void) {
    int minutes = 275;
    int cents = 1234;
    int number = 1234;

    int hours = minutes / 60;
    int mins_left = minutes % 60;
    int dollars = cents / 100;
    int cents_left = cents % 100;
    int last_digit = number % 10;

    printf("%d hours %d minutes\n", hours, mins_left);
    printf("%d dollars and %d cents\n", dollars, cents_left);
    printf("last digit of %d: %d\n", number, last_digit);
    return 0;
}
