# Testes de Carga, Tráfego HTTP e QoS

Este diretório contém a documentação, cenários e evidências dos testes de funcionamento, conexões persistentes, controle de admissão e mecanismos de Qualidade de Serviço (QoS) realizados no servidor HTTP/1.1.

---

## 1. Teste da Versão MVP1 (HTTP/1.1 Base e Persistência)

### 📊 Cenário do Teste Realizado
- **Ambiente:** WSL2 (Ubuntu / Linux) executando em CPU Intel Xeon E5-2470 v2.
- **Ferramenta de Captura:** Dumpcap / Wireshark 4.6.4.
- **Ferramenta de Teste:** Cliente `curl` efetuando requisições na porta `8080`.

### 🔍 Análise do Tráfego Capturado
Com base no arquivo de log gerado (`captura_fluxo.txt`), os seguintes eventos foram validados com sucesso:
1. **Início da Captura:** Registro do socket inicial mapeando a atividade em `localhost:8080`.
2. **Requisição de Arquivo de Texto:** Envio de `GET /teste.txt` com cabeçalho `Connection: keep-alive`. O servidor respondeu com `HTTP/1.1 200 OK` e entregou os 29 bytes do arquivo.
3. **Persistência de Conexão (Keep-Alive):** O tráfego do socket permaneceu aberto para múltiplas interações consecutivas (página principal e imagens como `/foto.jpg`), demonstrando que o reuso de conexões operou conforme a especificação do HTTP/1.1.
4. **Tratamento de Erro (404 Not Found):** Validação do tratamento de exceções ao responder corretamente um `HTTP/1.1 404 Not Found` de 55 bytes ao tentar acessar o arquivo `/404.html`.

---

## 2. Teste da Versão MVP2 (QoS, Controle de Admissão e Telemetria)

### 📊 Cenário do Teste Realizado
- **Ambiente:** WSL2 (Ubuntu / Linux) em ambiente virtualizado.
- **Ferramentas de Medição e Captura:** `curl`, `IPTraf-ng`, `tcpdump` e `Wireshark` (I/O Graphs).
- **Topologia:** Interface de loopback (`lo`) na porta `8080`.
- **Carga de Trabalho:** Página de teste (`galeria.html`) e imagens estáticas de grande dimensão (vários megabytes).

### 🔍 Testes Executados e Evidências Validadas

#### Teste 2.1: Validação do Controle de Admissão (Erro 503)
- **Objetivo:** Verificar se o servidor rejeita novas conexões quando a capacidade global configurada é ultrapassada.
- **Configuração:** Servidor iniciado com teto de `500 kbps` (`./servidor 500`), enquanto o IP `127.0.0.1` exigia uma alocação de `4200 kbps`.
- **Resultado:** A requisição do cliente foi rejeitada com `HTTP/1.1 503 Service Unavailable`, cabeçalho `Connection: close` e corpo HTML `<h1>503 - Capacidade Excedida</h1>`, garantindo que a carga adicional não comprometa as conexões já admitidas.

#### Teste 2.2: Conexões Simultâneas e Compartilhamento de Banda no Mesmo IP
- **Objetivo:** Validar a divisão equitativa de taxa entre fluxos paralelos originados do mesmo endereço IP.
- **Configuração:** Taxa base do IP `127.0.0.1` fixada em `4200 kbps` em `config/taxas.txt`. Disparo simultâneo de dois downloads com `curl` para imagens binárias.
- **Resultado:** O painel registrou 2 conexões ativas para o IP. A velocidade média de cada download convergiu para ~257.5 KB/s (≈ 2060 kbps), comprovando a divisão exata da taxa contratada (4200 kbps / 2 = 2100 kbps).

#### Teste 2.3: Atendimento Concorrente a Clientes Heterogêneos
- **Objetivo:** Garantir a segregação independente de parâmetros de taxa para diferentes clientes.
- **Configuração:** Mapeamento em `config/taxas.txt` dos IPs `127.0.0.1` (420 kbps) e `127.0.0.2` (1000 kbps), sob capacidade global do servidor de `20000 kbps`.
- **Resultado:** O painel de monitoramento alocou os recursos de forma independente para cada host (somando `1420.00 kbps` alocados), validando o isolamento de regras de QoS por IP.

#### Teste 2.4: Estabilidade de Vazão com IPTraf-ng, TCPdump e Wireshark (I/O Graphs)
- **Objetivo:** Avaliar a cadência de envio em baixo nível sob modulação por `usleep`.
- **Configuração:** Download de imagem de 2.93 MB com limitação de `420 kbps`. Captura do tráfego executada com `sudo tcpdump -i lo port 8080 -w trafego_qos.pcap` (885 pacotes capturados).
- **Resultado:** No `IPTraf-ng` e nos gráficos de I/O do Wireshark (*Statistics → I/O Graphs*), a taxa de transferência manteve um patamar rigorosamente plano em `420.000 bits/s` (~52.5 KB/s), atestando a eficácia do algoritmo contra rajadas de tráfego.

#### Teste 2.5: Telemetria de RTT Fim-a-Fim em Tempo Real
- **Objetivo:** Medir a estimativa de tempo de ida e volta do cliente no carregamento de recursos da aplicação.
- **Configuração:** Acesso à página web contendo imagens referenciadas via navegação HTTP/1.1 persistente.
- **Resultado:** O servidor mediu com precisão o intervalo entre a requisição do documento HTML base e a chegada da solicitação do primeiro objeto estático (ex.: `RTT Fim-a-Fim: 20.48 ms`), exibindo a métrica atualizada no painel de monitoramento do terminal.

---

### 📂 Arquivos de Evidência
- [Resultados dos testes](./resultados)
- [Relatórios](../docs)