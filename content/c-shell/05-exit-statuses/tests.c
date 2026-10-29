#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int touched(void) { return access("/tmp/bm_ran", F_OK) == 0; }

int main(void) {
    alarm(4);
    unlink("/tmp/bm_ran");

    CHECK("exit status 0", exit_code(0 << 8) == 0, "got %d", exit_code(0));
    CHECK("exit status 5", exit_code(5 << 8) == 5, "got %d", exit_code(5 << 8));
    CHECK("exit status 255", exit_code(255 << 8) == 255, "got %d", exit_code(255 << 8));
    CHECK("killed by signal 9 gives 137", exit_code(9) == 137, "got %d", exit_code(9));
    CHECK("killed by signal 2 gives 130", exit_code(2) == 130, "got %d", exit_code(2));
    CHECK("segfault with core dump flag gives 139", exit_code(11 | 0x80) == 139, "got %d", exit_code(11 | 0x80));
    CHECK("stopped by SIGTSTP (20) gives 148", exit_code((20 << 8) | 0x7f) == 148, "got %d", exit_code((20 << 8) | 0x7f));
    CHECK("-1 stays -1", exit_code(-1) == -1, "got %d", exit_code(-1));

    char *const t[] = {"true", NULL};
    char *const f[] = {"false", NULL};
    char *const s3[] = {"sh", "-c", "exit 3", NULL};
    char *const k9[] = {"sh", "-c", "kill -9 $$", NULL};
    char *const miss[] = {"no-such-command-bm", NULL};
    char *const mark[] = {"sh", "-c", "touch /tmp/bm_ran", NULL};

    CHECK("; returns the status of the second command", run_list(f, s3, OP_SEQ) == 3, "got %d", run_list(f, s3, OP_SEQ));
    CHECK("; returns 0 when the second succeeds", run_list(s3, t, OP_SEQ) == 0, "got %d", run_list(s3, t, OP_SEQ));

    CHECK("true && exit 3 gives 3", run_list(t, s3, OP_AND) == 3, "got %d", run_list(t, s3, OP_AND));

    unlink("/tmp/bm_ran");
    int r = run_list(s3, mark, OP_AND);
    CHECK("exit 3 && touch skips the second command and gives 3", r == 3 && !touched(), "r = %d, ran = %d", r, touched());

    unlink("/tmp/bm_ran");
    r = run_list(t, mark, OP_AND);
    CHECK("true && touch runs the second command", r == 0 && touched(), "r = %d, ran = %d", r, touched());

    unlink("/tmp/bm_ran");
    r = run_list(t, mark, OP_OR);
    CHECK("true || touch skips the second command", r == 0 && !touched(), "r = %d, ran = %d", r, touched());

    unlink("/tmp/bm_ran");
    r = run_list(f, mark, OP_OR);
    CHECK("false || touch runs the second command", r == 0 && touched(), "r = %d, ran = %d", r, touched());

    r = run_list(f, s3, OP_OR);
    CHECK("false || exit 3 gives 3", r == 3, "got %d", r);

    r = run_list(miss, t, OP_OR);
    CHECK("a missing command (127) counts as failure for ||", r == 0, "got %d", r);

    r = run_list(k9, t, OP_AND);
    CHECK("a killed command gives 137 and stops &&", r == 137, "got %d", r);

    r = run_list(t, k9, OP_SEQ);
    CHECK("a killed second command gives 137", r == 137, "got %d", r);
    return 0;
}
