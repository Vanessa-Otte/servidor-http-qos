# Documento de Requisitos — Servidor HTTP/1.1 com QoS

**Projeto:** Servidor HTTP com mecanismos de Qualidade de Serviço (QoS)
**Disciplina:** Fundamentos e Avaliação de Redes de Computadores — 2026/2
**Equipe:** Lorhan Santos de Lima e Vanessa Manzke Otte
**Versão do documento:** 2.0 — 16/10/2026

## Objetivo

Definir requisitos verificáveis para o servidor web, cobrindo o MVP1 e os incrementos de QoS.

## Escopo e contexto

O sistema deverá ser implementado em **C com Pthreads**, em ambiente Linux, e permitir a análise experimental de HTTP e desempenho de rede. O MVP1 concentra-se em servidor HTTP/1.1 com conexões persistentes e atendimento concorrente. Os incrementos seguintes incorporam controle de taxa por endereço IP, controle de admissão, estimativas de desempenho e visualização em tempo real.

## Requisitos funcionais

| ID | Requisito | Prioridade / versão | Critério de aceitação | Estado observado |
|---|---|---|---|---|
| RF-01 | O operador deve conseguir iniciar o servidor em Linux, escutando na porta configurada. | Obrigatório — MVP1 | Processo inicia, associa a porta e aceita conexão TCP; erro de inicialização é informado. A porta 8080 é a decisão atualmente documentada/codificada. | Implementado (parcial): `main()` cria socket, faz `bind()`/`listen()` na porta fixa 8080; tratamento de alguns erros de socket ainda é incompleto. |
| RF-02 | O servidor deve receber requisições HTTP/1.1 para recursos estáticos por meio de clientes HTTP. | Obrigatório — MVP1 | Requisição válida para recurso existente recebe status, cabeçalhos e corpo correspondentes. O conjunto de métodos aceitos deve ser definido; recomenda-se começar por `GET`. | Implementado (parcial): extrai método e URI, mas não valida o método nem implementa parser HTTP completo. |
| RF-03 | O caminho `/` deve corresponder ao documento inicial `index.html`. | Obrigatório — MVP1 | `GET /` retorna o mesmo conteúdo servido por `GET /index.html`. | Implementado no código. |
| RF-04 | Para recurso existente, o servidor deve responder `200 OK`, informar `Content-Length` e um `Content-Type` compatível com o tipo conhecido. | Obrigatório — MVP1 | Cliente recebe corpo completo e cabeçalhos coerentes para HTML, TXT, JPG/JPEG e PNG; demais extensões usam tipo genérico ou decisão documentada. | Implementado (parcial): cabeçalhos são gerados para extensões conhecidas; `send()` não trata envio parcial/erro. |
| RF-05 | Para recurso inexistente, o servidor deve responder `404 Not Found` com corpo de erro e comprimento declarado. | Obrigatório — MVP1 | Uma URI inexistente produz status 404 e resposta HTTP bem formada; a conexão segue a política keep-alive/close vigente. | Implementado (parcial): há resposta 404; parsing e envio HTTP ainda não são robustos a I/O parcial. |
| RF-06 | O servidor deve manter a conexão HTTP/1.1 persistente e processar requisições sucessivas na mesma conexão, salvo pedido de fechamento ou encerramento pelo cliente. | Obrigatório — MVP1 | Duas ou mais requisições/respostas são observadas no mesmo fluxo TCP; `Connection: close` encerra após a resposta. | Implementado (parcial): laço por conexão e busca textual por `Connection: close`; o relatório registra validação Wireshark. O reconhecimento é sensível a caixa/forma do cabeçalho e falta timeout de ociosidade definido. |
| RF-07 | O servidor deve atender clientes simultâneos sem serializar todo o atendimento em uma única thread. | Obrigatório — MVP1 | Vários clientes fazem requisições sobrepostas e recebem respostas completas e independentes. | Implementado (parcial): uma thread destacada por conexão; ainda não há limite de recursos nem controle de erro de `pthread_create()`. |
| RF-08 | O servidor deve transmitir arquivos binários e textuais sem alterar os bytes do conteúdo. | Obrigatório — MVP1 | Comparação de hash/tamanho do arquivo original e recebido coincide para TXT e imagem. | Implementado (parcial): leitura binária por blocos de 4096 bytes; envio parcial ainda não tratado. |
| RF-09 | Para objetos diferentes de HTML, o servidor deve limitar a taxa de envio conforme o endereço IP de origem e a taxa definida em arquivo auxiliar de configuração. | Obrigatório — MVP2 | Cliente cadastrado recebe objetos não HTML sob o limite configurado; o limite não é aplicado ao HTML. Formato, unidade e semântica do arquivo de configuração devem ser documentados. | Pendente: `mvp2/` não contém implementação/configuração. |
| RF-10 | Para IP não cadastrado, o servidor deve tentar atender objetos não HTML a 1000 kbps, conforme viabilidade indicada na especificação. | Obrigatório — MVP2 | Teste com IP ausente da tabela demonstra aplicação do padrão de 1000 kbps ou registra tecnicamente a inviabilidade/limitação. | Pendente. A unidade (kbps decimal ou Ki-bit/s) e a interpretação de “se viável” devem ser registradas. |
| RF-11 | Conexões simultâneas originadas do mesmo IP devem compartilhar a taxa previamente definida para esse endereço, em vez de cada uma receber o limite integral. | Obrigatório — MVP2 | Com N fluxos concorrentes do mesmo IP, a soma da vazão de objetos não HTML não excede sustentadamente o orçamento do IP; política de divisão deve ser documentada. | Pendente. |
| RF-12 | O operador deve informar na inicialização o limite máximo de taxa do servidor para o controle de admissão. | Obrigatório — MVP2 | Limite aceito como parâmetro, validado e exibido no início; unidade e valores inválidos são tratados. | Pendente. |
| RF-13 | O servidor deve controlar a admissão de clientes para que a carga admitida não exceda o limite máximo de taxa configurado. | Obrigatório — MVP2 | Sob carga acima do limite, o sistema aplica política explícita de admissão (por exemplo, fila ou rejeição), mantém a carga dentro do orçamento e informa o resultado ao cliente/operador. | Pendente. A especificação não determina fila, rejeição, status HTTP nem algoritmo; decisão necessária. |
| RF-14 | O servidor deve estimar tempo fim a fim/RTT e largura de banda do cliente a partir de duas requisições consecutivas do mesmo cliente no carregamento inicial (HTML e primeiro objeto referenciado). | Obrigatório — MVP2 | Para cenário controlado, servidor identifica as duas requisições do mesmo cliente, calcula e registra estimativas com unidades e algoritmo documentados. | Pendente. O intervalo observado no servidor é uma aproximação, não RTT de rede puro; fórmula/precisão ainda precisam ser especificadas. |
| RF-15 | O operador deve visualizar em tempo real os clientes atendidos e as estimativas de tempo/RTT e largura de banda disponíveis. | Obrigatório — MVP2 | Durante transferências simultâneas, painel/saída atualiza os clientes ativos e seus valores; ao encerrar, o cliente deixa de constar como ativo. | Pendente. Interface e frequência de atualização não definidas; saída de terminal é uma opção simples compatível com a atividade. |
| RF-16 | O servidor deve atender requisições de vários clientes/navegadores e arquivos referenciados numa página, permitindo observar o comportamento com clientes e acessos concorrentes. | Obrigatório — validação experimental | Teste com HTML e vários recursos, em clientes concorrentes, conclui sem perda/corrupção e permite captura e medição. | Parcial no MVP1; QoS ainda pendente. |

## Requisitos não funcionais

| ID | Requisito | Prioridade / critério verificável | Estado / observação |
|---|---|---|---|
| RNF-01 | A implementação deverá usar linguagem C e Pthreads em ambiente Linux. | Obrigatório. Compilação e execução em Linux documentadas. | O MVP1 usa C/Pthreads e o relatório descreve Linux/WSL Ubuntu. |
| RNF-02 | As respostas devem respeitar o enquadramento HTTP/1.1 necessário ao escopo: linha de status, cabeçalhos terminados por CRLF, `Content-Length` correto e semântica coerente de persistência. | Obrigatório. Validar com `curl --http1.1`, navegador e captura de pacotes. | Parcial; resposta simples existe, mas o parser de requisições é limitado. |
| RNF-03 | A implementação deve tratar leituras/escritas parciais, desconexões e erros de socket sem corromper resposta ou encerrar o processo de forma inesperada. | Importante. Testes de interrupção e comparação de conteúdo devem passar. | Pendente/limitação visível: retorno de `send()` é ignorado e `recv()` é tratado como se entregasse uma requisição completa. |
| RNF-04 | Entradas HTTP e caminhos de arquivo devem ser validados com limites seguros; o cliente não pode acessar arquivos fora da raiz pública. | Importante. Teste de URI com `..`, espaços, entradas longas e método não suportado não pode causar leitura fora de `www/`, overflow nem queda. | Pendente: `sscanf()` sem largura em vetores fixos e ausência de normalização/validação do caminho. |
| RNF-05 | O uso de recursos deve permanecer controlado sob conexões concorrentes e conexões ociosas. | Obrigatório para confiabilidade do controle de admissão; estabelecer carga-alvo e política/timeouts antes da validação. | Parcial: threads destacadas são criadas sem teto no MVP1; a especificação futura prevê controle de admissão. |
| RNF-06 | A limitação de taxa deve ser mensurável, estável e aplicada ao objeto não HTML sem penalizar indevidamente HTML. | Obrigatório — QoS. Registrar método de medição, janela e tolerância acordada pela equipe. | Pendente. |
| RNF-07 | O sistema deve fornecer evidência reprodutível de comportamento e desempenho com ferramentas de análise de rede e vazão. | Obrigatório. Incluir comandos, ambiente, capturas Wireshark/tcpdump/IPtraf e resultados no relatório da versão. | Relatório MVP1 descreve captura Wireshark; estrutura `testes/resultados/` ainda está vazia no clone analisado. |
| RNF-08 | Instalação, compilação, execução, configuração e limitações devem estar descritas em documentação versionada. | Importante. Um integrante deve reproduzir do zero seguindo o README. | Parcial; README do MVP1 possui instrução de execução com caractere `` ` `` extra após `./servidor`. |
| RNF-09 | A arquitetura e os artefatos devem permitir rastrear requisitos, casos de uso, testes e incrementos Scrum. | Importante. IDs consistentes entre documentos e registro dos resultados por sprint. | Este documento cria a base; ainda não foi incluído histórico de testes para QoS. |
| RNF-10 | O comportamento deve ser avaliado em ambiente experimental, preferencialmente com clientes por rede cabeada e por Wi-Fi, conforme disponibilidade. | Obrigatório como orientação da especificação. Registrar topologia, interface, endereços e limitações. | Pendente para versões futuras; relatório MVP1 usa loopback. |

## Relação com os demais artefatos

- Casos de uso: `docs/casos-de-uso.md`.
- Plano de testes e matriz de rastreabilidade: `docs/plano-de-testes.md`.
- Evidências propostas: `testes/resultados/`; scripts propostos: `testes/scripts/`.