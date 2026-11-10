static long tid(Task *t) { return t ? t->id : -1; }

void kmain(void) {
    WaitQueue q, r;
    Task t[12];
    Task *w;
    int rc;

    for (int i = 0; i < 12; i++)
        t[i] = (Task){ i, READY };
    memset(&q, 0xAB, sizeof q);
    memset(&r, 0x5C, sizeof r);
    wait_init(&q);
    wait_init(&r);

    w = wake_one(&q);
    CHECK("wake_one on a freshly initialised queue returns NULL", w == NULL,
          "returned task %ld (wait_init must reset head and count, whatever they held)", tid(w));

    rc = sleep_on(&q, &t[0]);
    CHECK("sleep_on a READY task returns 0", rc == 0, "returned %d", rc);
    CHECK("sleep_on sets the task BLOCKED", t[0].state == BLOCKED, "state is %d, expected BLOCKED (%d)", t[0].state, BLOCKED);
    w = wake_one(&q);
    CHECK("wake_one returns the sleeping task", w == &t[0], "returned task %ld, expected 0", tid(w));
    CHECK("wake_one sets the task READY", t[0].state == READY, "state is %d, expected READY (%d)", t[0].state, READY);
    w = wake_one(&q);
    CHECK("the queue is empty again after waking its only task", w == NULL, "returned task %ld", tid(w));

    sleep_on(&q, &t[1]); sleep_on(&q, &t[2]); sleep_on(&q, &t[3]);
    long o1 = tid(wake_one(&q)), o2 = tid(wake_one(&q)), o3 = tid(wake_one(&q));
    CHECK("tasks wake in the order they slept (FIFO)", o1 == 1 && o2 == 2 && o3 == 3,
          "woke %ld, %ld, %ld; expected 1, 2, 3", o1, o2, o3);

    int ok = 1;
    for (int i = 0; i < 8; i++)
        if (sleep_on(&q, &t[i]) != 0) ok = 0;
    CHECK("a queue holds 8 tasks", ok, "one of the first 8 sleep_on calls failed");
    rc = sleep_on(&q, &t[8]);
    CHECK("sleep_on on a full queue returns -1", rc == -1, "returned %d", rc);
    CHECK("a rejected task stays READY", t[8].state == READY, "task 8 state is %d", t[8].state);

    rc = sleep_on(&r, &t[0]);
    CHECK("a task that is already BLOCKED cannot sleep on a second queue", rc == -1, "returned %d", rc);
    w = wake_one(&r);
    CHECK("the rejected sleep left the other queue empty", w == NULL, "queue r returned task %ld", tid(w));

    long got[8];
    for (int i = 0; i < 5; i++) got[i] = tid(wake_one(&q));
    CHECK("the full queue wakes 0..4 first, in order", got[0] == 0 && got[1] == 1 && got[2] == 2 && got[3] == 3 && got[4] == 4,
          "woke %ld %ld %ld %ld %ld", got[0], got[1], got[2], got[3], got[4]);

    ok = 1;
    for (int i = 8; i < 12; i++)
        if (sleep_on(&q, &t[i]) != 0) ok = 0;
    sleep_on(&q, &t[0]);
    CHECK("after waking 5, five more tasks fit (the ring wraps around)", ok && t[0].state == BLOCKED,
          "a sleep_on after wrap-around failed");
    rc = sleep_on(&q, &t[1]);
    CHECK("the wrapped queue is full again at 8 tasks", rc == -1, "a 9th sleep_on returned %d", rc);

    long expect[8] = { 5, 6, 7, 8, 9, 10, 11, 0 };
    int bad = -1;
    for (int i = 0; i < 8; i++) {
        got[i] = tid(wake_one(&q));
        if (got[i] != expect[i] && bad < 0) bad = i;
    }
    CHECK("FIFO order holds across the wrap-around", bad < 0,
          "wake number %d returned task %ld, expected %ld", bad, bad < 0 ? 0 : got[bad], bad < 0 ? 0 : expect[bad]);
    w = wake_one(&q);
    CHECK("the wrapped queue ends empty", w == NULL, "returned task %ld", tid(w));

    sleep_on(&q, &t[2]);
    sleep_on(&r, &t[3]);
    sleep_on(&q, &t[4]);
    long r1 = tid(wake_one(&r)), q1 = tid(wake_one(&q)), r2 = tid(wake_one(&r)), q2 = tid(wake_one(&q));
    CHECK("two queues keep separate waiters", r1 == 3 && q1 == 2 && r2 == -1 && q2 == 4,
          "woke r:%ld q:%ld r:%ld q:%ld, expected r:3 q:2 r:none q:4", r1, q1, r2, q2);
}
