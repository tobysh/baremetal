#include <stdio.h>

int main(void) {
    int a = 7, b = 8, c = 10;
    double celsius = 36.6;

    double average = (a + b + c) / 3.0;
    double fahrenheit = celsius * 9.0 / 5.0 + 32;

    printf("Average: %.2f\n", average);
    printf("Fahrenheit: %.2f\n", fahrenheit);
    return 0;
}
