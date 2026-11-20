#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "utils.c"

#define REQUEST_BUFFER_SIZE 16384

#define STATUS_OK "200 OK"
#define STATUS_NOTFOUND "404 Not Found"
#define STATUS_ERR "500 Internal Server Error"
#define STATUS_TOOLARGE "413 Content Too Large"

#define EXT_HTML ".html"
#define EXT_CSS ".css"
#define EXT_JS ".js"

#define CTYPE_TXT "text/plain"
#define CTYPE_HTML "text/html"
#define CTYPE_CSS "text/css"
#define CTYPE_JS "application/javascript"

#define DIR_WWW "www"
#define DIR_BIN "/bin"
#define DIR_HTML "html"

#define FILENAME_INDEX "index.html"

#define ENVAR_CONTENT_LEN "CONTENT_LENGTH"

char* construct_response(char* status_code, char* content_type, char* body)
{
    printf("Response: %s\n", status_code);
    const char* FORMAT =
        "HTTP/1.1 %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "\r\n%s";
    
    int content_len = strlen(body);
    int total_len = content_len + strlen(int_to_str(content_len)) + strlen(status_code) + strlen(content_type) + strlen(FORMAT);
    char* response = malloc(total_len);
    snprintf(response, total_len, FORMAT, status_code, content_type, content_len, body);
    return response;
}

char* handle_bin(char* endpoint, char* request)
{
    str_prepend(endpoint, ".");
    char* body = strstr(request, "\r\n\r\n") + 4;
    setenv(ENVAR_CONTENT_LEN, int_to_str(strlen(body)), 1);
    int exit_code;
    char* response = execute_program(endpoint, body, &exit_code);
    if (exit_code != 0) return construct_response(STATUS_ERR, CTYPE_TXT, "ERROR");
    return response;
}

char* get_content_type(char* endpoint)
{
    char* filetype = get_filetype(endpoint);
    char* content_type = CTYPE_TXT;
    if (streq(filetype, EXT_HTML)) content_type = CTYPE_HTML;
    else if (streq(filetype, EXT_CSS)) content_type = CTYPE_CSS;
    else if (streq(filetype, EXT_JS)) content_type = CTYPE_JS;
    return content_type;
}

char* handle_html(char* endpoint)
{
    str_prepend(endpoint, DIR_HTML);
    if (is_dir(endpoint)) strcat(endpoint, FILENAME_INDEX);
    
    FILE* file = fopen(endpoint, "r");
    if (!file) return construct_response(STATUS_NOTFOUND, CTYPE_TXT, "NOT FOUND");
    char* body = read_file(file);
    fclose(file);
    
    char* content_type = get_content_type(endpoint);
    return construct_response(STATUS_OK, content_type, body);
}

char* handle_request(char* request)
{
    char* endpoint = get_endpoint(request);
    printf("\nRequest: %s\n", endpoint);
    if (str_startswith(endpoint, DIR_BIN)) return handle_bin(endpoint, request);
    return handle_html(endpoint);
}

void handle_connection(SSL* ssl, int client)
{
    char request_buffer[REQUEST_BUFFER_SIZE];
    int request_buffer_len = sizeof(request_buffer);
    int recv_len = -1;
    if (ssl) recv_len = SSL_read(ssl, request_buffer, request_buffer_len);
    else recv_len = recv(client, request_buffer, request_buffer_len, 0);
    
    char* response_buffer = NULL;
    if (recv_len < request_buffer_len)
    {
        request_buffer[recv_len] = '\0';
        response_buffer = handle_request(request_buffer);
    }
    else response_buffer = construct_response(STATUS_TOOLARGE, CTYPE_TXT, "TOO LARGE");
    
    if (response_buffer != NULL)
    {
        int response_buffer_len = strlen(response_buffer);
        if (ssl) SSL_write(ssl, response_buffer, response_buffer_len);
        else send(client, response_buffer, response_buffer_len, 0);
    }
}

SSL_CTX* init_ssl(char* filename_fullchain, char* filename_privkey)
{
    OPENSSL_init_ssl(0, NULL);
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_use_certificate_chain_file(ctx, filename_fullchain);
    SSL_CTX_use_PrivateKey_file(ctx, filename_privkey, SSL_FILETYPE_PEM);
    if (!SSL_CTX_check_private_key(ctx)) return NULL;
    return ctx;
}

int start_server(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sock;
}

int main(int argc, char* argv[])
{
    printf("*** Husky ***\n");

    SSL_CTX* ssl_ctx = NULL;
    if (argc > 2) ssl_ctx = init_ssl(argv[2], argv[3]);
    int sock = start_server(atoi(argv[1]));
    
    chdir(DIR_WWW);
    
    SSL* ssl = NULL;
    bool loop = true;
    while (loop)
    {
        ERR_print_errors_fp(stderr);
        fflush(stdout);
        if (ssl) SSL_free(ssl);
        ssl = NULL;
        
        listen(sock, 1);
        int client = accept(sock, NULL, NULL);
        
        if (ssl_ctx)
        {
            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, client);
            if (SSL_accept(ssl) <= 0)
            {
                close(client);
                continue;
            }
        }
        
        handle_connection(ssl, client);
        close(client);
    }
    return 0;
}
