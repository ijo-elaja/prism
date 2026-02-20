#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

char* read_file(char* path) {
    FILE* f = fopen(path, "r");
    guard(f, "Unable to open file: %s", path);

    char* buf = 0;
    ulong length;
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    guard(length > 0, "Failed to get file length");
    fseek(f, 0, SEEK_SET);
    buf = malloc(length + 1);
    guardf(buf, "Unable to malloc buffer of size %lu", f, length);
    fread(buf, 1, length, f);
    fclose(f);

    buf[length] = '\0';

    return buf;
}

char* read_fd(int fd, size_t buffer_size) {
    guardc(buffer_size >= 0, "Buffer size must be >= 0", fd);

    char* buffer = make(char, buffer_size);
    read(fd, buffer, buffer_size);

    return buffer;
}

char* sread_u(char** buffer_ptr, const char* until) {
    if(!buffer_ptr || !*buffer_ptr) return NULL;

    char* buffer = *buffer_ptr;
    char* result = strstr(buffer, until);
    if(!result) return NULL;

    size_t substr_len = result - buffer;
    char* new_buffer = malloc(substr_len + 1);
    if(!new_buffer) return NULL;

    memcpy(new_buffer, buffer, substr_len);
    new_buffer[substr_len] = '\0';

    // Advance the caller's buffer past the matched substring
    *buffer_ptr = result + strlen(until);

    return new_buffer;
}

char* sread_una(char** buffer_ptr, const char* until) {
    if(!buffer_ptr || !*buffer_ptr) return NULL;

    char* buffer = *buffer_ptr;
    char* result = strstr(buffer, until);
    if(!result) return NULL;

    size_t substr_len = result - buffer;
    char* new_buffer = malloc(substr_len + 1);
    if(!new_buffer) return NULL;

    memcpy(new_buffer, buffer, substr_len);
    new_buffer[substr_len] = '\0';

    // Don't advanced the caller's buffer

    return new_buffer;
}

bool smatch(const char* str1, const char* str2) {
    return strlen(str1) == strlen(str2) && strcmp(str1, str2) == 0 ? true : false;
}

char* mstrdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if(copy) memcpy(copy, s, len);
    return copy;
}

void strrev(char* str) {
    int l = 0;
    int r = strlen(str) - 1;
    char t;

    while(l < r) {
        t = str[l];
        str[l] = str[r];
        str[r] = t;

        l++;
        r--;
    }
}

void strrev_n(char* str, size_t len) {
    size_t l = 0;
    size_t r = len - 1;
    char t;

    while(l < r) {
        t = str[l];
        str[l] = str[r];
        str[r] = t;

        l++;
        r--;
    }
}

char* to_hex(char* str) {
    size_t len = strlen(str);
    size_t buffer_size = len * 2 + 2; // add 0x
    char* buffer = make(char, buffer_size + 1); // add \0
    buffer[0] = '0';
    buffer[1] = 'x';

    for(size_t i = 0;i < len;i++) {
        snprintf(buffer + 2 + i * 2, 3, "%02x", (unsigned char)str[i]);
    }

    buffer[buffer_size] = '\0';
    return buffer;
}

char* to_hex_u(char* str) {
    size_t len = strlen(str);
    size_t buffer_size = len * 2;
    char* buffer = make(char, buffer_size + 1); // add \0

    for(size_t i = 0;i < len;i++) {
        snprintf(buffer + i * 2, 3, "%02x", (unsigned char)str[i]);
    }

    buffer[buffer_size] = '\0';
    return buffer;
}

char* to_hex_n(char* str, size_t len) {
    size_t buffer_size = len * 2 + 2; // add 0x
    char* buffer = make(char, buffer_size + 1); // add \0
    buffer[0] = '0';
    buffer[1] = 'x';

    for(size_t i = 0;i < len;i++) {
        snprintf(buffer + 2 + i * 2, 3, "%02x", (unsigned char)str[i]);
    }

    buffer[buffer_size] = '\0';
    return buffer;
}

char* to_hex_nu(char* str, size_t len) {
    size_t buffer_size = len * 2;
    char* buffer = make(char, buffer_size + 1); // add \0

    for(size_t i = 0;i < len;i++) {
        snprintf(buffer + i * 2, 3, "%02x", (unsigned char)str[i]);
    }

    buffer[buffer_size] = '\0';
    return buffer;
}

void hex_table(const char* data, size_t len, bool show_labels) {
    if(!data || len == 0) return;

    // Print column headers if labels are enabled
    if(show_labels) {
        printf("       ");
        for(int col = 0; col < 16; col++) {
            printf("%02x ", col);
        }
        printf("\n");
    }

    // Print rows
    for(size_t row = 0; row < len; row += 16) {
        // Print row label if labels are enabled
        if(show_labels) {
            printf("%04zx: ", row);
        }

        // Print hex bytes
        for(int col = 0; col < 16; col++) {
            if(row + col < len) {
                printf("%02x ", (unsigned char)data[row + col]);
            } else if(show_labels) {
                printf("   ");
            }
        }

        printf("\n");
    }
}
