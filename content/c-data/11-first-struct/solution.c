#include <stdio.h>

typedef struct {
    int hours;
    int minutes;
} Time;

Time time_make(int hours, int minutes) {
    Time t = {hours, minutes};
    return t;
}

Time time_from_minutes(int total) {
    Time t = {total / 60, total % 60};
    return t;
}

int time_to_minutes(Time t) {
    return t.hours * 60 + t.minutes;
}

Time time_add(Time a, Time b) {
    return time_from_minutes(time_to_minutes(a) + time_to_minutes(b));
}

int time_before(Time a, Time b) {
    return time_to_minutes(a) < time_to_minutes(b);
}

int main(void) {
    Time t = time_add(time_make(1, 45), time_make(0, 30));
    printf("%d:%02d\n", t.hours, t.minutes);
    return 0;
}
