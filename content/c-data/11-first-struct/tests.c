int main(void) {
    Time t = time_make(9, 30);
    CHECK("time_make(9, 30) has the right fields", t.hours == 9 && t.minutes == 30, "got %d:%d", t.hours, t.minutes);
    Time z = time_make(0, 0);
    CHECK("time_make(0, 0)", z.hours == 0 && z.minutes == 0, "got %d:%d", z.hours, z.minutes);

    Time f = time_from_minutes(130);
    CHECK("time_from_minutes(130) is 2:10", f.hours == 2 && f.minutes == 10, "got %d:%d", f.hours, f.minutes);
    Time g = time_from_minutes(59);
    CHECK("time_from_minutes(59) is 0:59", g.hours == 0 && g.minutes == 59, "got %d:%d", g.hours, g.minutes);
    Time h = time_from_minutes(60);
    CHECK("time_from_minutes(60) is 1:00", h.hours == 1 && h.minutes == 0, "got %d:%d", h.hours, h.minutes);
    Time big = time_from_minutes(1505);
    CHECK("time_from_minutes does not wrap at 24 hours", big.hours == 25 && big.minutes == 5, "got %d:%d", big.hours, big.minutes);

    CHECK("time_to_minutes(2:10) is 130", time_to_minutes(time_make(2, 10)) == 130, "got %d", time_to_minutes(time_make(2, 10)));
    CHECK("time_to_minutes(0:00) is 0", time_to_minutes(z) == 0, "got %d", time_to_minutes(z));
    CHECK("time_to_minutes(0:45) is 45", time_to_minutes(time_make(0, 45)) == 45, "got %d", time_to_minutes(time_make(0, 45)));

    Time s = time_add(time_make(1, 45), time_make(0, 30));
    CHECK("time_add carries minutes into hours", s.hours == 2 && s.minutes == 15, "got %d:%d", s.hours, s.minutes);
    Time s2 = time_add(time_make(3, 10), time_make(4, 20));
    CHECK("time_add without carry", s2.hours == 7 && s2.minutes == 30, "got %d:%d", s2.hours, s2.minutes);
    Time s3 = time_add(time_make(0, 59), time_make(0, 1));
    CHECK("time_add reaches an exact hour", s3.hours == 1 && s3.minutes == 0, "got %d:%d", s3.hours, s3.minutes);
    Time a = time_make(5, 5);
    Time b = time_make(1, 1);
    Time c = time_add(a, b);
    CHECK("time_add does not change its inputs", a.hours == 5 && a.minutes == 5 && b.hours == 1 && b.minutes == 1 && c.hours == 6 && c.minutes == 6, "inputs changed");

    CHECK("time_before: earlier hour", time_before(time_make(1, 50), time_make(2, 10)) == 1, "expected 1");
    CHECK("time_before: later hour", time_before(time_make(2, 10), time_make(1, 50)) == 0, "expected 0");
    CHECK("time_before: same hour, minutes decide", time_before(time_make(3, 5), time_make(3, 6)) == 1, "expected 1");
    CHECK("time_before: equal times are not before", time_before(time_make(3, 5), time_make(3, 5)) == 0, "expected 0");
    return 0;
}
