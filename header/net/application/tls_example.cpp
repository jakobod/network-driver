#include <cstring>
#include <openssl/err.h>
#include <openssl/ssl.h>

int main(int argc, char** argv) {
  int sock;
  SSL_CTX* ctx;

  ctx = create_context();

  configure_context(ctx);

  sock = create_socket(4433);

  /* Handle connections */
  while (1) {
    struct sockaddr_in addr;
    unsigned int len = sizeof(addr);
    SSL* ssl;
    const char reply[] = "test\n";

    int client = accept(sock, (struct sockaddr*) &addr, &len);
    if (client < 0) {
      perror("Unable to accept");
      exit(EXIT_FAILURE);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);

    if (SSL_accept(ssl) <= 0) {
      ERR_print_errors_fp(stderr);
    } else {
      SSL_write(ssl, reply, strlen(reply));
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client);
  }

  close(sock);
  SSL_CTX_free(ctx);
}
