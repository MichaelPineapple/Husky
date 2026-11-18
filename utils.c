#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

bool str_startswith(const char* str, const char* prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

char* get_endpoint(char* request)
{
    char* endpoint = malloc(strlen(request));
    strcpy(endpoint, strchr(request, '/'));
    int terminator = strchr(endpoint, ' ') - endpoint;
    endpoint[terminator] = '\0';
    return endpoint;
}

char* int_to_str(int x)
{
    char* str = malloc(20);
    snprintf(str, sizeof(str), "%d", x);
    return str;
}

char* get_filetype(char* path)
{
    return strchr(path, '.');
}

bool streq(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL) return true;
    if (str1 == NULL || str2 == NULL) return false;
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

char* execute_program(const char* filename, const char* input)
{
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) return NULL;
    pid_t pid = fork();
    if (pid == -1) return NULL;
    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        execl(filename, filename, (char*)NULL);
        exit(1);
    }
    close(in_pipe[0]);
    close(out_pipe[1]);
    write(in_pipe[1], input, strlen(input));
    close(in_pipe[1]);
    char* output = malloc(1);
    size_t size = 0;
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
    {
        output = realloc(output, size + bytes_read + 1);
        memcpy(output + size, buffer, bytes_read);
        size += bytes_read;
    }
    close(out_pipe[0]);
    output[size] = '\0';
    waitpid(pid, NULL, 0);
    return output;
}
