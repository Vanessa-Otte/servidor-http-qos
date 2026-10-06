#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA 8080

void* atenderCliente(void* argumentoCliente) {
    int *ponteiroID = (int *)argumentoCliente;
    int idCliente = *ponteiroID;
    free(ponteiroID);

    pthread_detach(pthread_self());

    int manterConexao = 1;

    while (manterConexao == 1) {
        char buffer[4096] = {0};
        ssize_t bytesLidos = recv(idCliente, buffer, sizeof(buffer) - 1, 0);

        if (bytesLidos <= 0) {
            break; 
        }

        printf("Nova requisição recebida no socket %d:\n%s\n", idCliente, buffer);

        char metodo[16] = {0};
        char caminho[256] = {0};
        char caminhoArquivo[260] = {0};

        sscanf(buffer, "%s %s", metodo, caminho);

        if (strcmp(caminho, "/") == 0) {
            strcpy(caminho, "/index.html");
        }

        snprintf(caminhoArquivo, sizeof(caminhoArquivo), "./www%s", caminho);

        if (strstr(buffer, "Connection: close") != NULL) {
            manterConexao = 0;
        }

        FILE *arquivo = fopen(caminhoArquivo, "rb");

        if (arquivo == NULL) {
            char *msg404 = "<html><body><h1>404 - Nao Encontrado</h1></body></html>";
            char cabecalho404[512];

            snprintf(cabecalho404, sizeof(cabecalho404),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: text/html; charset=UTF-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: %s\r\n\r\n%s",
                     strlen(msg404),
                     (manterConexao ? "keep-alive" : "close"),
                     msg404);

            send(idCliente, cabecalho404, strlen(cabecalho404), 0);
        } 
        else {
            fseek(arquivo, 0, SEEK_END);
            long tamanhoArquivo = ftell(arquivo);
            rewind(arquivo);

            char *tipoConteudo = "application/octet-stream";
            if (strstr(caminho, ".html") != NULL) {
                tipoConteudo = "text/html; charset=UTF-8";
            } else if (strstr(caminho, ".jpg") != NULL || strstr(caminho, ".jpeg") != NULL) {
                tipoConteudo = "image/jpeg";
            } else if (strstr(caminho, ".png") != NULL) {
                tipoConteudo = "image/png";
            } else if (strstr(caminho, ".txt") != NULL) {
                tipoConteudo = "text/plain; charset=UTF-8";
            }

            char cabecalho200[512];
            snprintf(cabecalho200, sizeof(cabecalho200),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: %s\r\n\r\n",
                     tipoConteudo,
                     tamanhoArquivo,
                     (manterConexao ? "keep-alive" : "close"));

            send(idCliente, cabecalho200, strlen(cabecalho200), 0);

            char bufferEnvio[4096];
            size_t bytesLidosDisco;
            while ((bytesLidosDisco = fread(bufferEnvio, 1, sizeof(bufferEnvio), arquivo)) > 0) {
                send(idCliente, bufferEnvio, bytesLidosDisco, 0);
            }

            fclose(arquivo); 
        }
    }

    close(idCliente);
    return NULL;
}

int main() {
    int idServidor = socket(AF_INET, SOCK_STREAM, 0);
    int opcao = 1;
    setsockopt(idServidor, SOL_SOCKET, SO_REUSEADDR, &opcao, sizeof(opcao));

    struct sockaddr_in endereco;
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORTA);

    if (bind(idServidor, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
    perror("Erro no bind! Porta já em uso");
    exit(1); 
    }
    listen(idServidor, 10);
    printf("Servidor aguardando conexões na porta %d...\n", PORTA);

    while (1) {
        struct sockaddr_in enderecoCliente;
        socklen_t tamanhoEndereco = sizeof(enderecoCliente);

        int idCliente = accept(idServidor, (struct sockaddr *)&enderecoCliente, &tamanhoEndereco);

        if (idCliente < 0) {
            perror("Erro no Aceite");
            continue;
        }

        int *idClienteCopia = malloc(sizeof(int));
        *idClienteCopia = idCliente;

        pthread_t idThread;
        pthread_create(&idThread, NULL, atenderCliente, idClienteCopia);
    }

    close(idServidor);
    return 0;
}