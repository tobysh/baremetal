#include <stdio.h>

int main(void) {
    int a = 7, b = 8, c = 10;
    double celsius = 36.6;

    double average = (a + b + c) / 3;   // something is wrong here
    double fahrenheit = 0;              // fix me

    printf("Average: %.2f\n", average);
    printf("Fahrenheit: %.2f\n", fahrenheit);
    return 0;
}
