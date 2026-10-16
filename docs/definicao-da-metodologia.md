# Definição da Metodologia

A metodologia de engenharia de software adotada pela equipe será o **Scrum**.

No contexto deste trabalho, cada sprint terá 14 dias, e o incremento entregue será um **Produto Mínimo Viável (MVP)**. Essa escolha está alinhada à especificação do trabalho, que exige três versões MVP com intervalos de 14 dias, sendo a primeira um servidor HTTP/1.1 com conexões persistentes e atendimento concorrente, e a segunda a incorporação de mecanismos de QoS.

O Scrum foi escolhido por permitir:
* Entregas parciais e contínuas;
* Adaptação de escopo a cada sprint;
* Acompanhamento visual das tarefas;
* Divisão clara de papéis, mesmo em equipe reduzida;
* Registro formal dos avanços por meio de relatórios e código versionado.

## Como o Scrum será utilizado no projeto

### Artefatos

* Product Backlog: Lista priorizada de todos os requisitos do servidor HTTP com QoS. Inclui funcionalidades como conexões persistentes, threads, parser HTTP, tratamento de 404, controle de taxa por IP, controle de admissão, estimativa de RTT, largura de banda e visualização em tempo real.
* Sprint Backlog: Subconjunto do Product Backlog selecionado para a sprint atual, com tarefas detalhadas e prazos.
* Incremento: Código-fonte versionado no GitHub, relatório da versão e evidências de teste (capturas Wireshark, medições de vazão, etc.).

### Definição de Pronto (Definition of Done)

Uma tarefa só é considerada concluída quando:
* O código compila e executa sem erros;
* Os testes funcionais foram realizados;
* O comportamento foi validado com ferramentas de análise (Wireshark, tcpdump, etc.);
* O readme e o relatório da versão foram atualizados;
* O código foi commitado no GitHub;
* O card correspondente foi movido para “Concluído” no Trello.

## Organização no Trello

O quadro do Trello será organizado nas seguintes listas:
* Backlog
* Sprint Atual
* Em andamento
* Em revisão/teste
* Concluído

## Cronograma de Sprints
| Sprint | Data | Entrega |
|--- |--- |--- |
|Sprint 0 | 22/09 | Documento de metodologia e cronograma. |
|Sprint 1 | 05/10 |	MVP1: servidor HTTP/1.1 concorrente com keep-alive.|
|Sprint 2|	19/10|	MVP2: QoS, taxa por IP e controle de admissão.|
|Sprint 3|	26/10|	Validação da versão mais completa.|
|Sprint 4|	03/12|	Slides e seminário de socialização.|