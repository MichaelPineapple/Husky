#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

char* get_endpoint(char* request)
{
    char* endpoint = strchr(request, '/');
    int terminator = strchr(endpoint, ' ') - endpoint;
    endpoint[terminator] = '\0';
    return endpoint;
}

int int_strlen(int x)
{
    char str[20];
    snprintf(str, sizeof(str), "%d", x);
    return strlen(str);
}

char* get_filetype(char* path)
{
    return strchr(path, '.');
}

bool streq(const char* str1, const char* str2)
{
    return strcmp(str1, str2) == 0;
}

bool is_dir(const char* path)
{
    struct stat s;
    if (stat(path, &s) != 0) return 0;
    return (S_ISDIR(s.st_mode) == 1);
}

void str_prepend(char* str, const char* prefix)
{
    size_t len = strlen(prefix);
    memmove(str + len, str, strlen(str) + 1);
    memcpy(str, prefix, len);
}

char* read_file(FILE* file)
{
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = malloc(length);
    fread(buffer, 1, length, file);
    return buffer;
}
