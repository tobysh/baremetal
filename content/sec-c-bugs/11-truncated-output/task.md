Implement `int build_label(char *out,size_t cap,const char *user,const char *host)` producing `user@host`. Return 1 on success. Return 0 if `out` is NULL, if either name is NULL or empty, or if the complete text plus its terminating zero does not fit in `cap`. On failure with `cap>0` set `out[0]=0`; with `cap==0` write nothing at all. Never leave a shortened label in `out`.

Tests try many capacities and check the bytes around the buffer. Keep the supplied function names and types.
