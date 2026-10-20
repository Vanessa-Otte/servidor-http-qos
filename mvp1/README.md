# MVP 1 — Servidor HTTP/1.1 Persistente

## integrantes
- Lorhan Santos de Lima
- Vanessa Manzke Otte

## Requisitos
Para compilar e executar o projeto, é necessário um ambiente Linux com suporte à linguagem C e à biblioteca POSIX Threads.
O projeto foi desenvolvido e testado em ambiente Linux/WSL Ubuntu.

## Compilação e Execução
A partir do diretório `mvp1`, execute:
```bash
gcc -pthread servidor.c -o servidor
```

Após a compilação, será gerado o executável servidor.

Execute o servidor com:
```bash
./servidor
```

O servidor aguarda conexões na porta 8080.

Para acessar a página disponibilizada pelo servidor, utilize um navegador.

## Declaração de autoria
Este projeto foi desenvolvido integralmente pela equipe, sem ajuda não autorizada de alunos não membros do projeto no processo de codificação.