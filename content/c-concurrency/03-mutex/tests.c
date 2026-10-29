typedef struct { Counter *c; long limit; long successes; int attempts; } Capper;

static void *capper(void *arg) {
    Capper *w = arg;
    long ok = 0;
    for (int i = 0; i < w->attempts; i++) ok += counter_add_if_below(w->c, 1, w->limit);
    w->successes = ok;
    return NULL;
}

static void *plain_adder(void *arg) {
    Job *job = arg;
    for (int i = 0; i < job->iters; i++) counter_add(job->counter, 3);
    return NULL;
}

int main(void) {
    Counter c;
    counter_init(&c);
    counter_add(&c, 5);
    counter_add(&c, -2);
    CHECK("counter_add and counter_get work on one thread", counter_get(&c) == 3, "got %ld", counter_get(&c));
    CHECK("counter_add_if_below adds when it fits", counter_add_if_below(&c, 7, 10) == 1 && counter_get(&c) == 10, "got value %ld", counter_get(&c));
    CHECK("counter_add_if_below refuses when it would pass the limit", counter_add_if_below(&c, 1, 10) == 0 && counter_get(&c) == 10, "got value %ld", counter_get(&c));
    counter_destroy(&c);

    Counter shared;
    counter_init(&shared);
    Job job = {&shared, 40000};
    pthread_t t[6];
    for (int i = 0; i < 6; i++) pthread_create(&t[i], NULL, plain_adder, &job);
    for (int i = 0; i < 6; i++) pthread_join(t[i], NULL);
    CHECK("six threads adding 3 forty thousand times each lose no updates", counter_get(&shared) == 6L * 40000 * 3, "expected %ld, got %ld", 6L * 40000 * 3, counter_get(&shared));
    counter_destroy(&shared);

    int good = 1;
    long got = 0;
    for (int rep = 0; rep < 5; rep++) {
        got = count_in_parallel(4, 50000);
        if (got != 200000) good = 0;
    }
    CHECK("count_in_parallel(4, 50000) returns 200000 every time", good, "last result was %ld", got);
    CHECK("count_in_parallel with one thread", count_in_parallel(1, 1000) == 1000, "got %ld", count_in_parallel(1, 1000));
    CHECK("count_in_parallel with zero iterations", count_in_parallel(3, 0) == 0, "got %ld", count_in_parallel(3, 0));

    Counter cap;
    counter_init(&cap);
    Capper w[4];
    for (int i = 0; i < 4; i++) w[i] = (Capper){&cap, 1000, 0, 5000};
    for (int i = 0; i < 4; i++) pthread_create(&t[i], NULL, capper, &w[i]);
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    long total_ok = w[0].successes + w[1].successes + w[2].successes + w[3].successes;
    CHECK("counter_add_if_below never goes past its limit under contention", counter_get(&cap) == 1000 && total_ok == 1000, "value %ld, successes %ld (both should be 1000)", counter_get(&cap), total_ok);
    counter_destroy(&cap);
    return 0;
}
