#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define PORTA 8080
#define MAX_CLIENTES 100
#define TAXA_PADRAO_KBPS 1000.0 // RF-10

// --- 1. ESTRUTURAS GLOBAIS ---
typedef struct {
    char ip[16];
    double taxaBaseKbps;
    int conexoesAtivas;
} RegraIP;

typedef struct {
    RegraIP regras[MAX_CLIENTES];
    int qtdRegras;
    double vazaoMaximaServidorKbps;
    double vazaoAlocadaAtualKbps;
    char ultimoLogRTT[128];
    pthread_mutex_t mutex;
} EstadoQoS;

EstadoQoS gQoS = {
    .qtdRegras = 0,
    .vazaoAlocadaAtualKbps = 0.0,
    .ultimoLogRTT = "Aguardando medicao de RTT...",
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// Estrutura para repassar dados do socket e do IP para a thread
typedef struct {
    int idCliente;
    char ipCliente[16];
} ClienteArgs;

// --- 2. GERENCIAMENTO DE QOS E ADMISSÃO ---
void carregarConfiguracaoQos(const char *caminho) {
    FILE *f = fopen(caminho, "r");
    if (!f) {
        printf("[QoS] Arquivo %s nao encontrado. IPs usarao %.0f kbps por padrao.\n", caminho, TAXA_PADRAO_KBPS);
        return;
    }
    char ip[16];
    double taxa;
    while (fscanf(f, "%15s %lf", ip, &taxa) == 2) {
        pthread_mutex_lock(&gQoS.mutex);
        if (gQoS.qtdRegras < MAX_CLIENTES) {
            strcpy(gQoS.regras[gQoS.qtdRegras].ip, ip);
            gQoS.regras[gQoS.qtdRegras].taxaBaseKbps = taxa;
            gQoS.regras[gQoS.qtdRegras].conexoesAtivas = 0;
            gQoS.qtdRegras++;
        }
        pthread_mutex_unlock(&gQoS.mutex);
    }
    fclose(f);
}

double obterTaxaBaseIP(const char *ip) {
    for (int i = 0; i < gQoS.qtdRegras; i++) {
        if (strcmp(gQoS.regras[i].ip, ip) == 0) {
            return gQoS.regras[i].taxaBaseKbps;
        }
    }
    return TAXA_PADRAO_KBPS;
}

int obterConexoesAtivasIP(const char *ip) {
    int count = 1;
    pthread_mutex_lock(&gQoS.mutex);
    for (int i = 0; i < gQoS.qtdRegras; i++) {
        if (strcmp(gQoS.regras[i].ip, ip) == 0) {
            if (gQoS.regras[i].conexoesAtivas > 0) {
                count = gQoS.regras[i].conexoesAtivas;
            }
            break;
        }
    }
    pthread_mutex_unlock(&gQoS.mutex);
    return count;
}

int admitirCliente(const char *ip, double *taxaBaseOut) {
    pthread_mutex_lock(&gQoS.mutex);
    double taxaBase = obterTaxaBaseIP(ip);

    if (gQoS.vazaoAlocadaAtualKbps + taxaBase > gQoS.vazaoMaximaServidorKbps) {
        pthread_mutex_unlock(&gQoS.mutex);
        return 0; // Rejeitado
    }

    gQoS.vazaoAlocadaAtualKbps += taxaBase;

    int achou = 0;
    for (int i = 0; i < gQoS.qtdRegras; i++) {
        if (strcmp(gQoS.regras[i].ip, ip) == 0) {
            gQoS.regras[i].conexoesAtivas++;
            achou = 1;
            break;
        }
    }
    if (!achou && gQoS.qtdRegras < MAX_CLIENTES) {
        strcpy(gQoS.regras[gQoS.qtdRegras].ip, ip);
        gQoS.regras[gQoS.qtdRegras].taxaBaseKbps = TAXA_PADRAO_KBPS;
        gQoS.regras[gQoS.qtdRegras].conexoesAtivas = 1;
        gQoS.qtdRegras++;
    }

    *taxaBaseOut = taxaBase;
    pthread_mutex_unlock(&gQoS.mutex);
    return 1; // Admitido
}

void liberarCliente(const char *ip, double taxaBase) {
    pthread_mutex_lock(&gQoS.mutex);
    gQoS.vazaoAlocadaAtualKbps -= taxaBase;
    if (gQoS.vazaoAlocadaAtualKbps < 0) gQoS.vazaoAlocadaAtualKbps = 0;

    for (int i = 0; i < gQoS.qtdRegras; i++) {
        if (strcmp(gQoS.regras[i].ip, ip) == 0) {
            if (gQoS.regras[i].conexoesAtivas > 0) {
                gQoS.regras[i].conexoesAtivas--;
            }
            break;
        }
    }
    pthread_mutex_unlock(&gQoS.mutex);
}

// cores bonitinhas pro painel de monitoramento ^^
#define ANSI_RESET            "\x1b[0m"
#define ANSI_COR_TITULO       "\x1b[1;36m" // Ciano Negrito
#define ANSI_COR_LABEL        "\x1b[0;35m" // Roxo/Magenta
#define ANSI_COR_VALOR        "\x1b[1;32m" // Verde Negrito
#define ANSI_COR_CABECALHO    "\x1b[1;33m" // Amarelo Negrito
#define ANSI_COR_BORDA        "\x1b[0;34m" // Azul

// --- 5. PAINEL DE MONITORAMENTO ---
void *threadPainel(void *arg) {
    while (1) {
        sleep(1);
        pthread_mutex_lock(&gQoS.mutex);
        system("clear");
        
		// Cabeçalho Principal do Painel
        printf(ANSI_COR_BORDA "╔═════════════════════════════════════════════════════════╗\n" ANSI_RESET);
        printf(ANSI_COR_BORDA "║" ANSI_COR_TITULO "       PAINEL DE MONITORAMENTO QoS - SERVIDOR HTTP       " ANSI_COR_BORDA "║\n" ANSI_RESET);
        printf(ANSI_COR_BORDA "╠═════════════════════════════════════════════════════════╣\n" ANSI_RESET);
        
        // Métricas Globais do Servidor
        printf(ANSI_COR_BORDA "║ " ANSI_COR_LABEL "Vazao Maxima do Servidor :" ANSI_COR_VALOR "   %8.2f kbps              " ANSI_COR_BORDA "║\n" ANSI_RESET, gQoS.vazaoMaximaServidorKbps);
        printf(ANSI_COR_BORDA "║ " ANSI_COR_LABEL "Vazao Alocada Atual      :" ANSI_COR_VALOR "   %8.2f kbps              " ANSI_COR_BORDA "║\n" ANSI_RESET, gQoS.vazaoAlocadaAtualKbps);
        
        // Cabeçalho da Tabela de Clientes
        printf(ANSI_COR_BORDA "╠══════════════════╦═════════════════╦════════════════════╣\n" ANSI_RESET);
        printf(ANSI_COR_BORDA "║" ANSI_COR_CABECALHO " IP CLIENTE       " ANSI_COR_BORDA "║" ANSI_COR_CABECALHO " CONEXOES ATIVAS " ANSI_COR_BORDA "║" ANSI_COR_CABECALHO " TAXA BASE (kbps)   " ANSI_COR_BORDA "║\n" ANSI_RESET);
        printf(ANSI_COR_BORDA "╠══════════════════╬═════════════════╬════════════════════╣\n" ANSI_RESET);
        
        // Listagem dos Clientes Ativos
        int temCliente = 0;
        for (int i = 0; i < gQoS.qtdRegras; i++) {
            if (gQoS.regras[i].conexoesAtivas > 0) {
                temCliente = 1;
                // Formatação manual com espaçamento fixo para isolar as cores ANSI do alinhamento da borda
                printf(ANSI_COR_BORDA "║" ANSI_RESET " %-16s " ANSI_COR_BORDA "║" ANSI_RESET " %-15d " ANSI_COR_BORDA "║" ANSI_COR_VALOR " %-18.2f " ANSI_COR_BORDA "║\n" ANSI_RESET,
                       gQoS.regras[i].ip,
                       gQoS.regras[i].conexoesAtivas,
                       gQoS.regras[i].taxaBaseKbps);
            }
        }
        
        // Mensagem caso a tabela esteja vazia
        if (!temCliente) {
            printf(ANSI_COR_BORDA "║" ANSI_RESET " Nenhum ativo     " ANSI_COR_BORDA "║" ANSI_RESET " -               " ANSI_COR_BORDA "║" ANSI_RESET " -                  " ANSI_COR_BORDA "║\n" ANSI_RESET);
        }
        
        // Fechamento da Tabela
        printf(ANSI_COR_BORDA "╚══════════════════╩═════════════════╩════════════════════╝\n" ANSI_RESET);
        
        // --- Exibição de Telemetria / RTT em tempo real ---
        printf("\n" ANSI_COR_CABECALHO " [TELEMETRIA EM TEMPO REAL]" ANSI_RESET "\n");
        printf(" Ultima medicao: " ANSI_COR_VALOR "%s" ANSI_RESET "\n\n", gQoS.ultimoLogRTT);
        
        
        pthread_mutex_unlock(&gQoS.mutex);
    }
    return NULL;
}

// --- ATENDIMENTO AO CLIENTE ---
void* atenderCliente(void* argumentoCliente) {
    ClienteArgs *args = (ClienteArgs *)argumentoCliente;
    int idCliente = args->idCliente;
    char ipCliente[16];
    strcpy(ipCliente, args->ipCliente);
    free(args);

    pthread_detach(pthread_self());

    double taxaBaseIP = TAXA_PADRAO_KBPS;

    // Aplicar Controle de Admissão
    if (!admitirCliente(ipCliente, &taxaBaseIP)) {
        char *msg503 = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 35\r\nConnection: close\r\n\r\n<h1>503 - Capacidade Excedida</h1>";
        send(idCliente, msg503, strlen(msg503), 0);
        close(idCliente);
        return NULL;
    }

    int manterConexao = 1;
    struct timeval t_html_req = {0}; // Fora do while para manter o estado
    int mediuRTT = 0;

    while (manterConexao == 1) {
        char buffer[4096] = {0};
        ssize_t bytesLidos = recv(idCliente, buffer, sizeof(buffer) - 1, 0);

        if (bytesLidos <= 0) break;

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

        int ehHTML = (strstr(caminho, ".html") != NULL || strcmp(caminho, "/index.html") == 0);

        // --- 4. ESTIMATIVA DE RTT ---
        struct timeval agora;
        gettimeofday(&agora, NULL);

        if (ehHTML) {
            t_html_req = agora;
        } else if (!mediuRTT && t_html_req.tv_sec > 0) {
            double delta_ms = (agora.tv_sec - t_html_req.tv_sec) * 1000.0 +
                              (agora.tv_usec - t_html_req.tv_usec) / 1000.0;
            mediuRTT = 1;
            pthread_mutex_lock(&gQoS.mutex);
            snprintf(gQoS.ultimoLogRTT, sizeof(gQoS.ultimoLogRTT),
                     "IP %s -> RTT Fim-a-Fim: %.2f ms", ipCliente, delta_ms);
            pthread_mutex_unlock(&gQoS.mutex);       
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

            // --- 3. QoS ---
            char bufferEnvio[4096];
            size_t bytesLidosDisco;

            int numConexoes = obterConexoesAtivasIP(ipCliente);
            double taxaConexaoKbps = taxaBaseIP / numConexoes; // RF-11: Divisão da taxa
            double bytesPorSeg = (taxaConexaoKbps * 1000.0) / 8.0;

            while ((bytesLidosDisco = fread(bufferEnvio, 1, sizeof(bufferEnvio), arquivo)) > 0) {
                struct timeval t1, t2;
                gettimeofday(&t1, NULL);

                ssize_t enviados = send(idCliente, bufferEnvio, bytesLidosDisco, 0);
                if (enviados <= 0) break;

                // Aplica o limite de taxa apenas se NÃO for documento HTML (RF-09)
                if (!ehHTML) {
                    gettimeofday(&t2, NULL);
                    
                    double tempoGastoUs = (t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_usec - t1.tv_usec);
                    double tempoIdealUs = ((double)enviados / bytesPorSeg) * 1000000.0;

                    if (tempoIdealUs > tempoGastoUs) {
                        usleep((useconds_t)(tempoIdealUs - tempoGastoUs));
                    }
                }
            }

            fclose(arquivo); 
        }
    }

    liberarCliente(ipCliente, taxaBaseIP);
    close(idCliente);
    return NULL;
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    if (argc < 2) {
        printf("Uso: %s <vazaoMaximaServidorKbps>\n", argv[0]);
        return 1;
    }

    gQoS.vazaoMaximaServidorKbps = atof(argv[1]);
    carregarConfiguracaoQos("config/taxas.txt");

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
    printf("Servidor HTTP QoS iniciado na porta %d (Capacidade: %.0f kbps)...\n", PORTA, gQoS.vazaoMaximaServidorKbps);

    pthread_t t_painel;
    pthread_create(&t_painel, NULL, threadPainel, NULL);

    while (1) {
        struct sockaddr_in enderecoCliente;
        socklen_t tamanhoEndereco = sizeof(enderecoCliente);

        int idCliente = accept(idServidor, (struct sockaddr *)&enderecoCliente, &tamanhoEndereco);

        if (idCliente < 0) {
            perror("Erro no Aceite");
            continue;
        }

        ClienteArgs *args = malloc(sizeof(ClienteArgs));
        args->idCliente = idCliente;
        inet_ntop(AF_INET, &(enderecoCliente.sin_addr), args->ipCliente, sizeof(args->ipCliente));

        pthread_t idThread;
        pthread_create(&idThread, NULL, atenderCliente, args);
    }

    close(idServidor);
    return 0;
}