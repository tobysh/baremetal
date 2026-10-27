static Task idle = {0}, a = {1}, b = {2}, c = {3};
static Task extra[8];

static void fresh(void) {
    memset(&idle, 0, sizeof idle); idle.id = 0;
    a.id = 1; b.id = 2; c.id = 3;
    sched_init(&idle);
}
static int id_of(Task *t) { return t ? t->id : -1; }

void kmain(void) {
    fresh();
    CHECK("after sched_init the idle task is current and running", sched_current() == &idle && idle.state == TASK_RUNNING, "current id %d, state %d", id_of(sched_current()), idle.state);
    CHECK("the clock starts at 0", sched_now() == 0, "now is %lu", (unsigned long)sched_now());
    CHECK("with no tasks sched_pick_next returns the idle task", sched_pick_next() == &idle && idle.state == TASK_RUNNING, "picked id %d", id_of(sched_current()));

    fresh();
    int r1 = sched_add(&a), r2 = sched_add(&b), r3 = sched_add(&c);
    CHECK("sched_add returns 0 and marks tasks ready", r1 == 0 && r2 == 0 && r3 == 0 && a.state == TASK_READY && b.state == TASK_READY, "returns %d %d %d, a.state %d", r1, r2, r3, a.state);

    Task *t = sched_pick_next();
    CHECK("the first pick from idle is the first task added", t == &a && a.state == TASK_RUNNING && sched_current() == &a, "picked id %d", id_of(t));
    CHECK("the idle task is not left running", idle.state != TASK_RUNNING, "idle state %d", idle.state);
    t = sched_pick_next();
    CHECK("the next pick is the next slot, and the previous task is ready again", t == &b && a.state == TASK_READY && b.state == TASK_RUNNING, "picked id %d, a.state %d", id_of(t), a.state);
    t = sched_pick_next();
    CHECK("then the third", t == &c, "picked id %d", id_of(t));
    t = sched_pick_next();
    CHECK("the rotation wraps around to the first", t == &a, "picked id %d", id_of(t));

    int counts[4] = {0, 0, 0, 0};
    for (int i = 0; i < 9; i++) counts[id_of(sched_pick_next())]++;
    CHECK("nine more picks give each task exactly three turns", counts[1] == 3 && counts[2] == 3 && counts[3] == 3, "turns: a %d, b %d, c %d", counts[1], counts[2], counts[3]);

    fresh();
    sched_add(&a);
    sched_pick_next();
    CHECK("with a single task, pick_next keeps choosing it", sched_pick_next() == &a && a.state == TASK_RUNNING, "state %d", a.state);

    fresh();
    sched_add(&a); sched_add(&b); sched_add(&c);
    sched_pick_next();       // a runs
    sched_sleep(3);          // a sleeps until tick 3
    CHECK("sched_sleep marks the current task sleeping until now + ticks", a.state == TASK_SLEEPING && a.wake_tick == 3, "state %d, wake_tick %lu", a.state, (unsigned long)a.wake_tick);
    CHECK("a sleeping task is skipped", sched_pick_next() == &b, "picked something other than b");
    CHECK("it is skipped again", sched_pick_next() == &c && sched_pick_next() == &b, "a sleeping task was picked");
    sched_tick(); sched_tick();
    CHECK("two ticks are not enough to wake a 3-tick sleeper", a.state == TASK_SLEEPING && sched_now() == 2, "state %d, now %lu", a.state, (unsigned long)sched_now());
    sched_tick();
    CHECK("the third tick wakes it", a.state == TASK_READY, "state %d", a.state);
    CHECK("and it takes its turn in the rotation", sched_pick_next() == &c && sched_pick_next() == &a, "wrong order after waking");

    fresh();
    sched_add(&a); sched_add(&b);
    sched_pick_next(); sched_sleep(2);
    sched_pick_next(); sched_sleep(5);
    CHECK("when every task sleeps the idle task runs", sched_pick_next() == &idle && idle.state == TASK_RUNNING, "idle did not run");
    sched_tick(); sched_tick();
    CHECK("when a sleeper wakes, the next pick leaves idle for it", sched_pick_next() == &a, "picked id %d", id_of(sched_current()));

    fresh();
    sched_add(&a); sched_add(&b);
    sched_pick_next();       // a
    sched_block();
    CHECK("sched_block marks the current task blocked", a.state == TASK_BLOCKED, "state %d", a.state);
    sched_pick_next();       // b
    CHECK("a blocked task is not scheduled", sched_pick_next() == &b, "a blocked task was picked");
    sched_tick(); sched_tick(); sched_tick();
    CHECK("ticks do not wake blocked tasks", a.state == TASK_BLOCKED, "state %d", a.state);
    sched_unblock(&a);
    CHECK("sched_unblock makes a blocked task ready", a.state == TASK_READY, "state %d", a.state);
    sched_unblock(&b);
    CHECK("sched_unblock leaves running tasks alone", b.state == TASK_RUNNING, "state %d", b.state);
    sched_pick_next();       // a again (b -> ready)
    sched_sleep(10);
    sched_unblock(&a);
    CHECK("sched_unblock leaves sleeping tasks alone", a.state == TASK_SLEEPING, "state %d", a.state);

    fresh();
    sched_add(&a); sched_add(&b); sched_add(&c);
    sched_pick_next(); sched_pick_next();   // b running
    sched_exit();
    CHECK("sched_exit marks the current task dead", b.state == TASK_DEAD, "state %d", b.state);
    CHECK("the rotation moves on to the next task", sched_pick_next() == &c, "picked id %d", id_of(sched_current()));
    CHECK("a dead task is never scheduled again", sched_pick_next() == &a && sched_pick_next() == &c && b.state == TASK_DEAD, "dead task came back");

    fresh();
    for (int i = 0; i < 8; i++) { extra[i].id = 10 + i; sched_add(&extra[i]); }
    Task d = {99};
    CHECK("sched_add returns -1 when every slot holds a live task", sched_add(&d) == -1, "should be -1");
    sched_pick_next();       // extra[0]
    sched_exit();
    CHECK("a dead task's slot can be reused", sched_add(&d) == 0 && d.state == TASK_READY, "add failed");
    int order_ok = 1;
    for (int i = 1; i < 8; i++) if (id_of(sched_pick_next()) != 10 + i) order_ok = 0;
    CHECK("the new task sits in the dead task's slot, so it comes last in this rotation", order_ok && sched_pick_next() == &d, "picked id %d", id_of(sched_current()));
}
