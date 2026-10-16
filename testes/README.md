# Testes de Carga e Tráfego HTTP

Este diretório contém as evidências de testes de conexões persistentes e concorrência realizados no servidor HTTP/1.1.

## Teste da Versão MVP1

### 📊 Cenário do Teste Realizado
* **Ambiente:** WSL2 (Ubuntu / Linux) executando em CPU Intel Xeon E5-2470 v2.
* **Ferramenta de Captura:** Dumpcap / Wireshark 4.6.4.
* **Ferramenta de Teste:** Cliente `curl` efetuando requisições na porta `8080`.

### 🔍 Análise do Tráfego Capturado
Com base no arquivo de log gerado (`captura_fluxo.txt`), os seguintes eventos foram validados com sucesso:

1. **Início da Captura:** Registro do socket inicial mapeando a atividade em `localhost:8080`.
2. **Requisição do Arquivo de Texto:** O cliente enviou um `GET /teste.txt` com cabeçalho `Connection: keep-alive`. O servidor respondeu com `HTTP/1.1 200 OK` e entregou os 29 bytes de conteúdo do arquivo.
3. **Persistência de Conexão (Keep-Alive):** O tráfego do socket permaneceu aberto para múltiplas interações consecutivas (como as requisições da página principal e arquivos de imagem `/foto.jpg`), demonstrando que o reuso de conexões do nosso servidor está operando conforme a especificação do HTTP/1.1.
4. **Tratamento de Erro (404):** Validação do tratamento de exceções do servidor ao responder corretamente um `HTTP/1.1 404 Not Found` de 55 bytes ao tentar acessar o arquivo `/404.html`.

### 📂 Arquivos de Evidência
- [Resultados dos testes](./resultados)