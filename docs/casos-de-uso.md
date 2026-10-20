# Casos de Uso — Servidor HTTP/1.1

## 1. Mapeamento de Atores

* **Cliente HTTP (Navegador / cURL / Wget):** Sistema ou processo cliente que estabelece conexões TCP, envia requisições HTTP/1.1 e consome recursos estáticos (HTML, TXT, imagens).
* **Operador do Servidor:** Desenvolvedor ou administrador que inicia o servidor via linha de comando, especifica os parâmetros de capacidade global e monitora a execução e métricas em tempo real.

---

## 2. Detalhamento dos Casos de Uso

### UC-01: Iniciar Servidor e Carregar Configuração de QoS
* **Ator Principal:** Operador do Servidor
* **Descrição:** O operador executa o servidor informando a capacidade máxima total em kbps. O sistema associa a porta 8080, inicializa as estruturas de sincronização (`pthread_mutex_t`) e lê a tabela de taxas por IP de `config/taxas.txt`.
* **Pré-condições:** Ambiente Linux funcional com permissão para vincular o socket na porta 8080.
* **Fluxo Principal:**
  1. Operador executa `./servidor <vazao_maxima_kbps>`.
  2. Sistema valida o argumento de linha de comando.
  3. Sistema lê o arquivo `config/taxas.txt` e armazena os pares `(IP, taxa_kbps)` na tabela global.
  4. Sistema cria o socket TCP, executa `bind()` na porta 8080 e entra em estado de escuta `listen()`.
  5. Sistema dispara a thread secundária responsável pelo painel de monitoramento.
* **Fluxo Alternativo (Parâmetro Ausente):** Se a vazão máxima não for informada, o servidor exibe mensagem de uso e encerra.
* **Fluxo Alternativo (Arquivo de Configuração Ausente):** Se `config/taxas.txt` não existir, o servidor exibe alerta e aplica a taxa padrão de 1000 kbps para todos os IPs.
* **Requisitos Associados:** RF-01, RF-09, RF-12.

---

### UC-02: Solicitar e Servir Recurso Estático Base (GET / ou /index.html)
* **Ator Principal:** Cliente HTTP
* **Descrição:** O cliente envia requisição `GET /` ou `GET /index.html`. O servidor responde com o código `200 OK`, define o `Content-Type` apropriado e transmite o arquivo HTML sem restrições de vazão.
* **Pré-condições:** Servidor em execução e cliente admitido no sistema.
* **Fluxo Principal:**
  1. Cliente envia requisição `GET / HTTP/1.1`.
  2. Servidor normaliza a URI para `/index.html`.
  3. Servidor localiza o arquivo no diretório público `./www/`.
  4. Servidor envia o cabeçalho `HTTP/1.1 200 OK` com `Content-Type: text/html` e `Content-Length`.
  5. Servidor lê o arquivo do disco e transmite o conteúdo em velocidade máxima de rede (sem aplicar `usleep`).
* **Requisitos Associados:** RF-02, RF-03, RF-04, RF-06, RF-08.

---

### UC-03: Tratar Recurso Inexistente (404 Not Found)
* **Ator Principal:** Cliente HTTP
* **Descrição:** O cliente solicita uma URI correspondente a um arquivo ausente no diretório `./www/`. O servidor responde com erro `404 Not Found` mantendo a política de persistência da conexão.
* **Pré-condições:** Conexão TCP ativa.
* **Fluxo Principal:**
  1. Cliente solicita arquivo inexistente (ex.: `GET /naoexiste.html`).
  2. Servidor falha ao abrir o arquivo via `fopen()`.
  3. Servidor envia resposta `HTTP/1.1 404 Not Found` com documento HTML explicativo e cabeçalho `Content-Length`.
  4. A conexão TCP permanece aberta caso o cabeçalho `Connection: keep-alive` esteja ativo.
* **Requisitos Associados:** RF-05, RF-06.

---

### UC-04: Transmitir Mídias/Objetos com Controle de Taxa (Traffic Shaping)
* **Ator Principal:** Cliente HTTP
* **Descrição:** O cliente solicita um recurso não-HTML (como imagens `.jpg` ou `.png`). O servidor limita a taxa de envio com base na tabela de QoS e divide o orçamento de banda se houver múltiplas conexões concorrentes do mesmo IP.
* **Pré-condições:** Cliente admitido no controle de admissão.
* **Fluxo Principal:**
  1. Cliente envia `GET /imagem1.jpg HTTP/1.1`.
  2. Servidor identifica que o recurso não é HTML.
  3. Servidor consulta a taxa do IP do cliente (ou usa 1000 kbps caso não cadastrado).
  4. Servidor verifica quantas conexões ativas existem para aquele IP e calcula `taxa_efetiva = taxa_base / conexoes_ativas`.
  5. Servidor envia os blocos do arquivo inserindo pausas com `usleep()` para cadenciar o envio sob o limite calculado.
* **Requisitos Associados:** RF-08, RF-09, RF-10, RF-11, RNF-06.

---

### UC-05: Aplicar Controle de Admissão de Carga (503 Service Unavailable)
* **Ator Principal:** Cliente HTTP
* **Descrição:** Uma nova conexão é estabelecida, mas a soma da taxa cadastrada do cliente com a vazão atualmente alocada ultrapassa a capacidade máxima configurada para o servidor. A solicitação é rejeitada.
* **Pré-condições:** Servidor sob alta carga ou com teto de vazão reduzido.
* **Fluxo Principal:**
  1. Cliente conecta via `accept()`.
  2. Servidor executa a rotina `admitir_cliente()` sob trava de mutex.
  3. Servidor detecta que `vazao_alocada + taxa_cliente > vazao_maxima`.
  4. Servidor despacha resposta `HTTP/1.1 503 Service Unavailable` com `Connection: close`.
  5. Servidor encerra o socket do cliente sem processar requisições adicionais.
* **Requisitos Associados:** RF-12, RF-13, RNF-05.

---

### UC-06: Estimar RTT Fim-a-Fim do Cliente
* **Ator Principal:** Cliente HTTP / Servidor
* **Descrição:** O servidor afere o tempo decorrido entre a solicitação do documento HTML inicial e a chegada da requisição do primeiro objeto estático referenciado na mesma sessão.
* **Pré-condições:** Cliente acessando página HTML com mídias embutidas em conexão persistente.
* **Fluxo Principal:**
  1. Cliente solicita `GET /index.html`.
  2. Servidor registra o timestamp $T_1$ (`gettimeofday`).
  3. Cliente processa o HTML e solicita a primeira imagem `GET /imagem1.jpg`.
  4. Servidor registra o timestamp $T_2$.
  5. Servidor calcula $\Delta T = T_2 - T_1$ e armazena o RTT estimado do cliente.
* **Requisitos Associados:** RF-14, RF-16.

---

### UC-07: Visualizar Painel de Monitoramento em Tempo Real
* **Ator Principal:** Operador do Servidor
* **Descrição:** O operador visualiza no terminal a atualização contínua das métricas do servidor, incluindo capacidade máxima, vazão alocada, clientes ativos e estimativas de RTT.
* **Pré-condições:** Servidor ativo.
* **Fluxo Principal:**
  1. A thread do painel acorda a cada 1 segundo.
  2. Sob trava de mutex, lê a tabela global de QoS e clientes ativos.
  3. Limpa o terminal e renderiza a interface com formatação ANSI.
  4. Exibe lista de IPs, conexões ativas, taxas base e telemetria.
* **Requisitos Associados:** RF-15.
