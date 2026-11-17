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

char* handle_request(char* request)
{
    return "Hello, World!";
}

void handle_connection(SSL* ssl, int client)
{
    char request_buffer[1024];
    if (ssl) SSL_read(ssl, request_buffer, sizeof(request_buffer));
    else recv(client, request_buffer, sizeof(request_buffer), 0);
    printf("REQUEST:\n%s\n\n", request_buffer);
    
    char response_buffer[1024];
    char* body = handle_request(request_buffer);
    
    const char* FORMAT =
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n\r\n%s";
    
    snprintf(response_buffer, sizeof(response_buffer), FORMAT, strlen(body), body);
    
    if (ssl) SSL_write(ssl, response_buffer, strlen(response_buffer));
    else send(client, response_buffer, strlen(response_buffer), 0);
    
    printf("RESPONSE:\n%s\n\n", response_buffer);
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
