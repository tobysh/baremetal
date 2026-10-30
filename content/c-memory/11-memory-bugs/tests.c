/* Counting allocator. Defining malloc and free here replaces the C library's
   versions for the whole program, so the learner's calls are observed too. */
extern void *__libc_malloc(size_t n);
extern void __libc_free(void *p);

typedef struct { void *ptr; size_t size; int live; } BmBlock;
static BmBlock bm_blocks[512];
static int bm_nblocks = 0;
static int bm_tracking = 0;
static int bm_double_frees = 0;
static int bm_frees = 0;

void *malloc(size_t n) {
    void *p = __libc_malloc(n);
    if (bm_tracking && p != NULL && bm_nblocks < 512) {
        bm_blocks[bm_nblocks].ptr = p;
        bm_blocks[bm_nblocks].size = n;
        bm_blocks[bm_nblocks].live = 1;
        bm_nblocks++;
    }
    return p;
}

void free(void *p) {
    if (p == NULL) return;
    if (bm_tracking) {
        for (int i = bm_nblocks - 1; i >= 0; i--) {
            if (bm_blocks[i].ptr == p) {
                if (!bm_blocks[i].live) { bm_double_frees++; return; }
                memset(p, 0xDD, bm_blocks[i].size);   /* scribble: use after free reads nonsense */
                bm_blocks[i].live = 0;
                bm_frees++;
                return;      /* block is deliberately never reused */
            }
        }
    }
    __libc_free(p);
}

void *calloc(size_t count, size_t size) {
    if (size != 0 && count > (size_t)-1 / size) return NULL;
    void *p = malloc(count * size);
    if (p != NULL) memset(p, 0, count * size);
    return p;
}

static void bm_reset(void) {
    bm_nblocks = 0; bm_double_frees = 0; bm_frees = 0;
    bm_tracking = 1;
}

static int bm_live(void) {
    int n = 0;
    for (int i = 0; i < bm_nblocks; i++) if (bm_blocks[i].live) n++;
    return n;
}

static int bm_allocs(void) { return bm_nblocks; }

int main(void) {
    /* give stdout a fixed buffer so the harness's own printing never allocates */
    static char bm_outbuf[16384];
    setvbuf(stdout, bm_outbuf, _IOFBF, sizeof bm_outbuf);
    /* make_label: exactly one block may survive, the result */
    bm_reset();
    char *label = make_label("Ada", 7);
    int live_after = bm_live();
    CHECK("make_label(\"Ada\", 7) returns \"Ada#7\"", label != NULL && strcmp(label, "Ada#7") == 0, "got \"%s\"", label ? label : "(null)");
    CHECK("make_label leaves only its result alive (no leaked scratch block)", live_after == 1, "%d blocks alive after the call, expected 1", live_after);
    free(label);
    CHECK("freeing the label releases everything", bm_live() == 0 && bm_double_frees == 0, "%d blocks still alive", bm_live());

    bm_reset();
    char *label2 = make_label("Grace Hopper", 1234);
    int live2 = bm_live();
    CHECK("make_label(\"Grace Hopper\", 1234) is \"Grace Hopper#1234\"", label2 != NULL && strcmp(label2, "Grace Hopper#1234") == 0, "got \"%s\"", label2 ? label2 : "(null)");
    CHECK("still no leak with a longer name", live2 == 1, "%d blocks alive after the call, expected 1", live2);
    free(label2);

    /* sum_and_free: right answer from live data, array freed exactly once */
    bm_reset();
    int *v = malloc(4 * sizeof *v);
    v[0] = 10; v[1] = 20; v[2] = 30; v[3] = 40;
    int s = sum_and_free(v, 4);
    CHECK("sum_and_free sums {10, 20, 30, 40} to 100 (data read while still alive)", s == 100, "got %d", s);
    CHECK("sum_and_free frees the array exactly once", bm_live() == 0 && bm_double_frees == 0, "%d blocks alive, %d double frees", bm_live(), bm_double_frees);

    bm_reset();
    int *w = malloc(3 * sizeof *w);
    w[0] = -5; w[1] = 2; w[2] = 1;
    s = sum_and_free(w, 3);
    CHECK("sum_and_free sums {-5, 2, 1} to -2", s == -2, "got %d", s);
    CHECK("the second array is freed too", bm_live() == 0, "%d blocks alive", bm_live());

    bm_reset();
    int *one = malloc(sizeof *one);
    one[0] = 99;
    s = sum_and_free(one, 1);
    CHECK("sum_and_free works for a one-element array", s == 99, "got %d", s);

    /* release_person: three blocks, each freed once */
    bm_reset();
    Person *p = create_person("Ada", 5);
    int allocs = bm_allocs();
    CHECK("create_person allocates three blocks (struct, name, scores)", allocs == 3, "%d allocations", allocs);
    release_person(p);
    CHECK("release_person never frees a block twice", bm_double_frees == 0, "%d double free(s)", bm_double_frees);
    CHECK("release_person frees every block create_person made (no leak)", bm_live() == 0, "%d block(s) still alive", bm_live());
    CHECK("release_person makes exactly three frees", bm_frees == 3, "%d frees", bm_frees);

    bm_reset();
    Person *q = create_person("Linus Torvalds", 100);
    Person *r = create_person("Margaret", 1);
    release_person(q);
    release_person(r);
    CHECK("releasing two people leaves nothing alive and frees nothing twice", bm_live() == 0 && bm_double_frees == 0, "%d alive, %d double frees", bm_live(), bm_double_frees);

    bm_tracking = 0;
    return 0;
}
