# Plano de Testes e Matriz de Rastreabilidade


## 1. Escopo e Estratégia de Testes

O plano de testes visa garantir a verificação funcional e não funcional do servidor web sob os incrementos **MVP 1** (concorrência e persistência HTTP/1.1) e **MVP 2** (mecanismos de QoS, controle de admissão, *traffic shaping* e telemetria de RTT).

### Ferramentas Utilizadas:
* **Injeção de Carga:** `curl`, `wget` e navegadores web (Mozilla Firefox / Google Chrome).
* **Análise de Tráfego e Pacotes:** Wireshark e `tcpdump`.
* **Monitoramento de Vazão:** `IPTraf-ng` na interface de loopback (`lo`) e ethernet/Wi-Fi.

---

## 2. Casos de Teste Estruturados

### TC-01: Inicialização do Socket e Binds de Rede
* **Objetivo:** Verificar se o servidor associa a porta 8080 e inicia o processo sem erros.
* **Procedimento:** Executar `./servidor 5000` no Linux/WSL.
* **Resultado Esperado:** Servidor vincula a porta 8080, exibe mensagem de inicialização e aguarda conexões via `accept()`.
* **Requisitos Mapeados:** RF-01, RNF-01.

### TC-02: Persistência de Conexão HTTP/1.1 (Keep-Alive)
* **Objetivo:** Confirmar o reuso da mesma conexão TCP para múltiplas requisições sucessivas.
* **Procedimento:** Requisitar `/index.html`, `/teste.txt` e `/foto.jpg` na mesma sessão via cURL/Navegador com Wireshark ativo.
* **Resultado Esperado:** Todas as trocas ocorrem dentro da mesma porta efêmera do cliente sem pacotes `FIN`/`RST` intermediários.
* **Requisitos Mapeados:** RF-02, RF-06, RNF-02.

### TC-03: Mapeamento da Rota Raiz e Entrega de Arquivo Estático
* **Objetivo:** Validar a entrega do `index.html` ao acessar `/`.
* **Procedimento:** Executar `curl -i http://localhost:8080/`.
* **Resultado Esperado:** Retorno `HTTP/1.1 200 OK`, cabeçalho `Content-Type: text/html` e corpo idêntico ao arquivo em `./www/index.html`.
* **Requisitos Mapeados:** RF-03, RF-04, RF-08.

### TC-04: Tratamento de Recurso Inexistente (404 Not Found)
* **Objetivo:** Validar resposta de erro para arquivo não encontrado.
* **Procedimento:** Executar `curl -i http://localhost:8080/arquivo_ausente.html`.
* **Resultado Esperado:** Retorno `HTTP/1.1 404 Not Found` com cabeçalho `Content-Length` e corpo HTML de erro.
* **Requisitos Mapeados:** RF-05.

### TC-05: Modulação de Taxa (Traffic Shaping) em Mídias não-HTML
* **Objetivo:** Garantir a limitação de velocidade em arquivos de imagem sem penalizar arquivos HTML.
* **Procedimento:** Baixar imagem binária de 5 MB configurada com limite de 500 kbps em `config/taxas.txt`. Monitorar no IPTraf-ng.
* **Resultado Esperado:** A velocidade de transferência da imagem estabiliza em ~62.5 KB/s (500 kbps), enquanto o HTML é entregue instantaneamente.
* **Requisitos Mapeados:** RF-09, RF-10, RNF-06.

### TC-06: Divisão Equitativa de Taxa entre Conexões Concorrentes do Mesmo IP
* **Objetivo:** Validar o compartilhamento dinâmico do orçamento de banda de um IP.
* **Procedimento:** Disparar 2 downloads simultâneos do mesmo IP configurado com 4200 kbps.
* **Resultado Esperado:** Cada conexão recebe aproximadamente 2100 kbps (~250 KB/s) no painel e no cURL.
* **Requisitos Mapeados:** RF-11.

### TC-07: Controle de Admissão de Carga (Erro 503)
* **Objetivo:** Garantir que conexões excedentes à capacidade global do servidor sejam rejeitadas.
* **Procedimento:** Iniciar servidor com `./servidor 500` (500 kbps) e conectar cliente cuja regra exige 4200 kbps.
* **Resultado Esperado:** O cliente recebe resposta `HTTP/1.1 503 Service Unavailable` com `Connection: close` e é desconectado.
* **Requisitos Mapeados:** RF-12, RF-13, RNF-05.

### TC-08: Telemetria de RTT e Painel Visual em Tempo Real
* **Objetivo:** Validar a medição de RTT fim-a-fim e atualização do painel.
* **Procedimento:** Acessar página contendo imagens e observar o terminal do servidor.
* **Resultado Esperado:** Servidor exibe RTT medido entre requisições consecutivas e atualiza a tabela de clientes a cada 1s.
* **Requisitos Mapeados:** RF-14, RF-15, RF-16.

---

## 3. Matriz de Rastreabilidade

| Requisito ID | Descrição Sumária | Versão / Escopo | Caso de Uso | Caso de Teste |
| :--- | :--- | :--- | :--- | :--- |
| **RF-01** | Inicialização em Linux e bind na porta 8080 | MVP 1 | UC-01 | TC-01 |
| **RF-02** | Recebimento de requisições HTTP/1.1 estáticas | MVP 1 | UC-02 | TC-02 |
| **RF-03** | Mapeamento da rota `/` para `index.html` | MVP 1 | UC-02 | TC-03 |
| **RF-04** | Resposta 200 OK, Content-Length e Content-Type | MVP 1 | UC-02 | TC-03 |
| **RF-05** | Resposta 404 Not Found para arquivo inexistente | MVP 1 | UC-03 | TC-04 |
| **RF-06** | Manutenção de conexão persistente (Keep-Alive) | MVP 1 | UC-02, UC-03 | TC-02 |
| **RF-07** | Atendimento concorrente multithread por conexão | MVP 1 | UC-02, UC-04 | TC-02, TC-06 |
| **RF-08** | Transmissão de arquivos sem alteração de bytes | MVP 1 | UC-02, UC-04 | TC-03, TC-05 |
| **RF-09** | Limitação de taxa por IP para objetos não-HTML | MVP 2 | UC-01, UC-04 | TC-05 |
| **RF-10** | Taxa padrão de 1000 kbps para IP não cadastrado | MVP 2 | UC-04 | TC-05 |
| **RF-11** | Divisão de taxa entre conexões do mesmo IP | MVP 2 | UC-04 | TC-06 |
| **RF-12** | Parâmetro de vazão máxima do servidor na CLI | MVP 2 | UC-01, UC-05 | TC-01, TC-07 |
| **RF-13** | Controle de admissão e resposta 503 | MVP 2 | UC-05 | TC-07 |
| **RF-14** | Estimativa de RTT e largura de banda do cliente | MVP 2 | UC-06 | TC-08 |
| **RF-15** | Painel visual de monitoramento em tempo real | MVP 2 | UC-07 | TC-08 |
| **RF-16** | Avaliação experimental sob carga de múltiplos objetos | MVP 2 | UC-06 | TC-08 |
| **RNF-01** | Linguagem C e Pthreads em ambiente Linux | MVP 1 / MVP 2 | Todos | TC-01 |
| **RNF-02** | Enquadramento e semântica HTTP/1.1 | MVP 1 / MVP 2 | UC-02 | TC-02 |
| **RNF-05** | Controle de uso de recursos sob conexões concorrentes | MVP 2 | UC-05 | TC-07 |
| **RNF-06** | Limitação de taxa mensurável e estável | MVP 2 | UC-04 | TC-05 |
| **RNF-07** | Evidências reprodutíveis com Wireshark/IPTraf-ng | MVP 1 / MVP 2 | Todos | TC-02, TC-05, TC-07 |
