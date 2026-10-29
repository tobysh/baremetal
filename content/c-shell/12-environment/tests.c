#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static int child_status(char *const argv[]) {
    pid_t pid = fork();
    if (pid == 0) { execvp(argv[0], argv); _exit(127); }
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static const char *ex(const char *in, int status, int *len) {
    static char out[256];
    *len = expand_word(in, out, sizeof out, status);
    return out;
}

int main(void) {
    alarm(4);
    int n;
    const char *r;
    setenv("BM_A", "alpha", 1);
    setenv("BM_B", "beta", 1);
    setenv("BM_EMPTY", "", 1);
    unsetenv("BM_UNSET");

    r = ex("plain", 0, &n);
    CHECK("a word without $ is copied", n == 5 && !strcmp(r, "plain"), "n = %d, out = %s", n, r);
    r = ex("$BM_A", 0, &n);
    CHECK("$NAME is replaced", n == 5 && !strcmp(r, "alpha"), "n = %d, out = %s", n, r);
    r = ex("x$BM_A/y$BM_B.", 0, &n);
    CHECK("references inside text", !strcmp(r, "xalpha/ybeta."), "out = %s", r);
    r = ex("${BM_A}ine", 0, &n);
    CHECK("${NAME} marks where the name ends", !strcmp(r, "alphaine"), "out = %s", r);
    r = ex("$BM_Aine", 0, &n);
    CHECK("$NAMEmore looks up the longer name", !strcmp(r, ""), "out = %s", r);
    r = ex("[$BM_UNSET][$BM_EMPTY]", 0, &n);
    CHECK("unset and empty variables expand to nothing", n == 4 && !strcmp(r, "[][]"), "n = %d, out = %s", n, r);
    r = ex("status=$?", 42, &n);
    CHECK("$? is the last status", !strcmp(r, "status=42"), "out = %s", r);
    r = ex("$?$?", 0, &n);
    CHECK("$? twice", !strcmp(r, "00"), "out = %s", r);
    r = ex("cost $5 and $ and $", 0, &n);
    CHECK("$ not followed by a name stays a literal $", !strcmp(r, "cost $5 and $ and $"), "out = %s", r);
    r = ex("${BM_A", 0, &n);
    CHECK("an unterminated ${ is copied literally", !strcmp(r, "${BM_A"), "out = %s", r);
    r = ex("${}", 0, &n);
    CHECK("${} is copied literally", !strcmp(r, "${}"), "out = %s", r);
    r = ex("", 0, &n);
    CHECK("empty input gives an empty string", n == 0 && !strcmp(r, ""), "n = %d", n);

    char small[8];
    n = expand_word("$BM_A$BM_B", small, sizeof small, 0);
    CHECK("a result that does not fit returns -1", n == -1, "n = %d", n);
    n = expand_word("$BM_A!!", small, sizeof small, 0);
    CHECK("a result that exactly fits works", n == 7 && !strcmp(small, "alpha!!"), "n = %d, out = %s", n, small);

    setenv("BM_X", "1", 1);
    CHECK("do_export sets a variable", do_export("BM_NEW=hello world") == 0 && !strcmp(getenv("BM_NEW"), "hello world"), "BM_NEW = %s", getenv("BM_NEW") ? getenv("BM_NEW") : "(null)");
    CHECK("do_export allows an empty value", do_export("BM_NEW=") == 0 && getenv("BM_NEW") && !strcmp(getenv("BM_NEW"), ""), "not set to empty");
    CHECK("do_export keeps '=' inside the value", do_export("BM_EQ=a=b") == 0 && !strcmp(getenv("BM_EQ"), "a=b"), "BM_EQ = %s", getenv("BM_EQ") ? getenv("BM_EQ") : "(null)");
    CHECK("do_export rejects a missing '='", do_export("BM_NOEQ") == 1 && !getenv("BM_NOEQ"), "accepted");
    CHECK("do_export rejects a name starting with a digit", do_export("1BAD=x") == 1, "accepted");
    CHECK("do_export rejects a bad character in the name", do_export("A-B=x") == 1 && do_export("=x") == 1, "accepted");

    do_export("BM_CHILD=from-shell");
    char *const check1[] = {"sh", "-c", "[ \"$BM_CHILD\" = from-shell ]", NULL};
    CHECK("a child process inherits exported variables", child_status(check1) == 0, "the child did not see BM_CHILD");

    CHECK("do_unset removes the variable", do_unset("BM_CHILD") == 0 && !getenv("BM_CHILD"), "still set");
    char *const check2[] = {"sh", "-c", "[ -z \"${BM_CHILD+set}\" ]", NULL};
    CHECK("... and children no longer see it", child_status(check2) == 0, "the child still sees BM_CHILD");
    CHECK("do_unset on an unset name is fine", do_unset("BM_NEVER_SET") == 0, "returned 1");
    CHECK("do_unset rejects an invalid name", do_unset("a b") == 1 && do_unset("") == 1, "accepted");

    char a[] = "BM_KEEP=value";
    char copy[sizeof a];
    strcpy(copy, a);
    do_export(a);
    CHECK("do_export does not modify its argument", !strcmp(a, copy), "argument became %s", a);
    return 0;
}
