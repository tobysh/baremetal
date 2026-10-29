#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

static int no_children_left(void) {
    errno = 0;
    return waitpid(-1, NULL, WNOHANG) == -1 && errno == ECHILD;
}

int main(void) {
    alarm(4);
    pid_t me = getpid();

    char *const t[] = {"true", NULL};
    char *const f[] = {"false", NULL};
    CHECK("true exits with 0", run_command(t) == 0, "got %d", run_command(t));
    CHECK("false exits with 1", run_command(f) == 1, "got %d", run_command(f));

    char *const s7[] = {"sh", "-c", "exit 7", NULL};
    CHECK("exit code 7 is reported", run_command(s7) == 7, "got %d", run_command(s7));

    char *const s200[] = {"sh", "-c", "exit 200", NULL};
    CHECK("exit code 200 is reported", run_command(s200) == 200, "got %d", run_command(s200));

    char *const args[] = {"sh", "-c", "[ \"$1\" = x ] && [ \"$2\" = 'y z' ] && exit 3; exit 4", "sh", "x", "y z", NULL};
    CHECK("arguments reach the program intact", run_command(args) == 3, "got %d", run_command(args));

    char *const missing[] = {"definitely-not-a-command-bm", NULL};
    CHECK("a missing command gives 127", run_command(missing) == 127, "got %d", run_command(missing));

    char *const killed[] = {"sh", "-c", "kill -9 $$", NULL};
    CHECK("a child killed by a signal gives -1", run_command(killed) == -1, "got %d", run_command(killed));

    CHECK("still the same process afterwards", getpid() == me, "pid changed");

    int ok = 1, r = 0;
    for (int i = 0; i < 20 && ok; i++) { r = run_command(t); ok = (r == 0); }
    CHECK("twenty runs in a row work", ok, "last result %d", r);

    CHECK("no zombie children are left behind", no_children_left(), "waitpid(-1) found another child");
    return 0;
}
