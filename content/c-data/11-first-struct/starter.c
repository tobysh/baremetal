#include <stdio.h>

typedef struct {
    int hours;
    int minutes;
} Time;

Time time_make(int hours, int minutes) {
    Time t = {0, 0};
    return t;
}

Time time_from_minutes(int total) {
    Time t = {0, 0};
    return t;
}

int time_to_minutes(Time t) {
    return 0;
}

Time time_add(Time a, Time b) {
    Time t = {0, 0};
    return t;
}

int time_before(Time a, Time b) {
    return 0;
}

// Optional: try them with Run.
int main(void) {
    Time t = time_add(time_make(1, 45), time_make(0, 30));
    printf("%d:%02d (expected 2:15)\n", t.hours, t.minutes);
    return 0;
}
