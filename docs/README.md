# Documentação do Projeto — Servidor HTTP/1.1

Esta pasta contém a documentação técnica, especificação de requisitos, definição metodológica e relatórios das entregas do projeto de Servidor HTTP/1.1.

---

## 📂 Estrutura e Sumário dos Documentos

### 1. Requisitos e Engenharia de Software
* **`documento-de-requisitos.md`**:
  * Consolida os Requisitos Funcionais (RF-01 a RF-16) e Não-Funcionais (RNF-01 a RNF-10) do servidor HTTP/1.1 em C com Pthreads.
  * Abrange desde o atendimento concorrente e persistência *Keep-Alive* (MVP 1) até o controle de taxa por IP, controle de admissão por capacidade global e telemetria de RTT em tempo real (MVP 2).

* **`definicao-da-metodologia.md`**:
  * Descreve a metodologia ágil **Scrum** adotada pela equipe (Sprints de 14 dias com entregas incrementais por Produto Mínimo Viável - MVP).
  * Detalha a organização do Trello, os artefatos (Product Backlog, Sprint Backlog e Incremento) e os critérios da *Definition of Done*.

* **`casos-de-uso.md`**:
  * Mapeia os casos de uso do sistema para o operador e para os clientes HTTP.

* **`plano-de-testes.md`**:
  * Apresenta o plano de testes funcionais e a matriz de rastreabilidade entre requisitos e cenários de validação.

---

### 2. Relatórios Técnicos das Sprints / MVPs

* **`Relatório_1_Trabalho_Prático_Redes.pdf`**:
  * Relatório do **MVP 1** (Entregue em 05/10).
  * Detalha a arquitetura concorrente do servidor HTTP/1.1 baseada em *Sockets Berkeley* e *Pthreads* em C para ambiente Linux.
  * Apresenta os resultados experimentais capturados no Wireshark, comprovando a eficácia do mecanismo *Keep-Alive* na redução de latência e reuso do *Three-Way Handshake* TCP.

* **`Relatório_2_Trabalho_Prático_Redes.pdf`**:
  * Relatório do **MVP 2** (Entregue em 19/10).
  * Documenta os quatro pilares de QoS:
    1. **Controle de Admissão:** Rejeição de novas conexões com `503 Service Unavailable` ao atingir a capacidade máxima global do servidor.
    2. **Modelagem de Tráfego:** Modulação de taxa de envio via `usleep` com divisão equitativa entre conexões simultâneas do mesmo IP.
    3. **Telemetria Fim-a-Fim:** Medição em tempo real do atraso de RTT da aplicação entre a requisição HTML e o primeiro objeto embutido.
    4. **Painel Operacional:** Thread dedicada para renderização do terminal em tempo real com escape ANSI.
  * Valida a implementação experimental com ferramentas de análise como `curl`, `IPTraf-ng`, `tcpdump` e gráficos de I/O do `Wireshark`.

---

## 👥 Equipe do Projeto
* **Lorhan Santos de Lima**
* **Vanessa Manzke Otte**
* **Disciplina:** Fundamentos e Avaliação de Redes de Computadores (UNIPAMPA — Bagé/RS)
