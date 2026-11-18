#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

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

char* execute_program(const char* filename, const char* input, int* exit_code)
{
    int in_pipe[2];
    int out_pipe[2];

    if (exit_code != NULL) *exit_code = -1;

    if (pipe(in_pipe) == -1) return NULL;

    if (pipe(out_pipe) == -1)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        return NULL;
    }

    char* output = malloc(1);

    if (output == NULL)
    {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return NULL;
    }

    output[0] = '\0';

    size_t size = 0;
    int memory_error = 0;
    int write_error = 0;
    int read_error = 0;

    pid_t pid = fork();

    if (pid == -1)
    {
        free(output);
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return NULL;
    }

    if (pid == 0)
    {
        close(in_pipe[1]);
        close(out_pipe[0]);

        if (dup2(in_pipe[0], STDIN_FILENO) == -1) _exit(126);
        if (dup2(out_pipe[1], STDOUT_FILENO) == -1) _exit(126);
        if (dup2(out_pipe[1], STDERR_FILENO) == -1) _exit(126);

        close(in_pipe[0]);
        close(out_pipe[1]);

        execl(filename, filename, (char*)NULL);

        perror(filename);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    const char* input_data = input ? input : "";
    size_t input_size = strlen(input_data);
    size_t bytes_written = 0;

    while (bytes_written < input_size)
    {
        ssize_t bytes = write(in_pipe[1], input_data + bytes_written, input_size - bytes_written);
        if (bytes > 0) bytes_written += bytes;
        else if (bytes == -1 && errno == EINTR) continue;
        else
        {
            write_error = 1;
            break;
        }
    }

    close(in_pipe[1]);

    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
    {
        if (memory_error) continue;

        char* new_output = realloc(output, size + bytes_read + 1);

        if (new_output == NULL)
        {
            free(output);
            output = NULL;
            memory_error = 1;
            continue;
        }

        output = new_output;

        memcpy(output + size, buffer, bytes_read);
        size += bytes_read;
        output[size] = '\0';
    }

    if (bytes_read == -1) read_error = 1;

    close(out_pipe[0]);

    int status;
    pid_t wait_result;

    do
    {
        wait_result = waitpid(pid, &status, 0);
    }
    while (wait_result == -1 && errno == EINTR);

    if (wait_result == -1)
    {
        free(output);
        return NULL;
    }

    if (exit_code != NULL)
    {
        if (WIFEXITED(status)) *exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) *exit_code = 128 + WTERMSIG(status);
    }

    if (memory_error || write_error || read_error)
    {
        free(output);
        return NULL;
    }

    return output;
}
