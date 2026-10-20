# MVP 2 — Servidor HTTP/1.1 com Qualidade de Serviço (QoS)

## Integrantes
- Lorhan Santos de Lima
- Vanessa Manzke Otte

### Funcionalidades do MVP 2
- **Controle de Taxa por IP:** Limitação de largura de banda aplicada ao envio de objetos não-HTML (como imagens), com compartilhamento dinâmico da taxa para múltiplas conexões simultâneas do mesmo IP.
- **Controle de Admissão:** Gerenciamento da capacidade total do servidor para não exceder a vazão máxima configurada na inicialização.
- **Estimativa de RTT Fim-a-Fim:** Medição do atraso entre a solicitação da página HTML e a do primeiro objeto estático referenciado.
- **Painel Visual em Tempo Real:** Exibição contínua no terminal dos clientes conectados, taxa alocada e métricas ativas.

---

## Requisitos
Para compilar e executar o projeto, é necessário um ambiente Linux com suporte à linguagem C e à biblioteca POSIX Threads.
O projeto foi desenvolvido e testado em ambiente Linux/WSL Ubuntu.

---

## Configuração de QoS (`config/taxas.txt`)
O arquivo `config/taxas.txt` define os limites de vazão por IP em **kbps**:
```text
127.0.0.1 500
192.168.1.50 2000
```
*Endereços IP não cadastrados utilizam a taxa padrão de **1000 kbps**.*

---

## Compilação e Execução

### Compilação
A partir do diretório `mvp2`, execute:
```bash
gcc -pthread servidor.c -o servidor
```

Após a compilação, será gerado o executável `servidor`.

### Execução
Execute o servidor informando a **vazão máxima total do servidor** em kbps. Exemplo:
```bash
./servidor 5000
```

O servidor aguarda conexões na porta **8080**.

---

## Declaração de autoria
Este projeto foi desenvolvido integralmente pela equipe, sem ajuda não autorizada de alunos não membros do projeto no processo de codificação.