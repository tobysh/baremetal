#include <stdio.h>

int main(void) {
    int minutes = 275;
    int cents = 1234;
    int number = 1234;

    int hours = 0;           // fix me
    int mins_left = 0;       // fix me
    int dollars = 0;         // fix me
    int cents_left = 0;      // fix me
    int last_digit = 0;      // fix me

    printf("%d hours %d minutes\n", hours, mins_left);
    printf("%d dollars and %d cents\n", dollars, cents_left);
    printf("last digit of %d: %d\n", number, last_digit);
    return 0;
}
