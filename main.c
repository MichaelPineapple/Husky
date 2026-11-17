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

#define PORT 8080

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

int start_server()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return sock;
}

char* handle_request(char* request)
{
    return "Hello, World!";
}

int main(int argc, char* argv[])
{
    printf("*** Husky Server ***\n");

    SSL_CTX* ssl_ctx = init_ssl(argv[1], argv[2]);
    if (!ssl_ctx) { ERR_print_errors_fp(stderr); return 1; }
    
    int sock = start_server();
    bool loop = true;
    while (loop)
    {
        listen(sock, 1);
        int client = accept(sock, NULL, NULL);
        
        SSL* ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client);
        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client);
            continue;
        }
        
        char req_buffer[1024];
        SSL_read(ssl, req_buffer, sizeof(req_buffer));
        printf("REQUEST:\n%s\n\n", req_buffer);
        char resp_buffer[1024];
        char* body = handle_request(req_buffer);
        
        const char* FORMAT =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n\r\n%s";
        
        snprintf(resp_buffer, sizeof(resp_buffer), FORMAT, strlen(body), body);
        SSL_write(ssl, resp_buffer, strlen(resp_buffer));
        printf("RESPONSE:\n%s\n\n", resp_buffer);
        fflush(stdout);
        close(client);
    }
    return 0;
}
