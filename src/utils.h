#ifndef PRISM_UTILS_H
#define PRISM_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>

static atomic_int GUARD_INDEX = 1;

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef size_t usize;

#ifdef DEBUG
#define debug(msg, ...) do { \
    fprintf(stderr, "[debug] " msg "\n", ##__VA_ARGS__); \
} while(0)
#else
#define debug(msg, ...)
#endif

#define log(msg, ...) do { \
    fprintf(stderr, "[info] " msg "\n", ##__VA_ARGS__); \
} while(0)
#define info(msg, ...) do { \
    fprintf(stderr, "[info] " msg "\n", ##__VA_ARGS__); \
} while(0)
#define _error(msg, ...) do { \
    fprintf(stderr, "[err] " msg "\n", ##__VA_ARGS__); \
} while(0)
#define error(msg, ...) do { _error(msg, ##__VA_ARGS__); } while(0)
#define guard(c, msg, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); exit(GUARD_INDEX); } GUARD_INDEX++; } while(0)
#define guardf(c, msg, fp, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); fclose(fp); exit(GUARD_INDEX); } GUARD_INDEX++; } while(0)
#define guardc(c, msg, fd, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); close(fd); exit(GUARD_INDEX); } GUARD_INDEX++; } while(0)
#define guardn(c, msg, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); return NULL; } } while(0)
#define guardv(c, msg, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); return; } } while(0)
#define guardv_nl(c) do { if(!(c)) { return; } } while(0)

#define try(c, msg, s, ...) do { if(!(c)) { _error(msg, ##__VA_ARGS__); } else { s; } } while(0)

#define make(t, ...) malloc(sizeof(t) * (((#__VA_ARGS__)[0] != '\0') ? __VA_ARGS__ : 1))

char* read_file(char* path);
char* read_fd(int fd, size_t buffer_size);

char* sread_u(char** buffer_ptr, const char* until);
char* sread_una(char** buffer_ptr, const char* until);
bool smatch(const char* str1, const char* str2);
char* mstrdup(const char* s);

void strrev(char* str);
void strrev_n(char* str, size_t len);
char* to_hex(char* str);
char* to_hex_u(char* str);
char* to_hex_n(char* str, size_t len);
char* to_hex_nu(char* str, size_t len);

void hex_table(const char* data, size_t len, bool show_labels);

#endif // PRISM_UTILS_H
