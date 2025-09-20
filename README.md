# Micro-Kernel com Jogo da Forca

Este projeto é um micro-kernel simples, desenvolvido para rodar em modo protegido de 32 bits (arquitetura x86). O kernel inicializa o hardware básico, configura o tratamento de interrupções para o teclado e executa um jogo da forca diretamente na tela, sem depender de um sistema operacional subjacente.

## Estrutura dos Arquivos

### `kernel.c`

Este é o arquivo principal do kernel, escrito em C. Ele contém toda a lógica de alto nível, incluindo a inicialização de interrupções, a manipulação da tela e a lógica do jogo.

**Funções Principais:**
*   `kmain()`: Ponto de entrada principal chamado pelo código assembly. Contém o loop principal do jogo.
*   `idt_init()`: Inicializa a Tabela de Descritores de Interrupção (IDT) e remapeia o PIC (Programmable Interrupt Controller) para habilitar as interrupções do teclado.
*   `keyboard_handler_main()`: Processa os scancodes recebidos do teclado, converte-os para caracteres e os armazena para uso no jogo.
*   `print_char()`, `print_string()`, `clear_screen()`: Funções para escrever diretamente na memória de vídeo (`0xb8000`), controlando o que é exibido na tela.
*   `reset_game()`, `draw_game_state()`, `check_guess()`: Funções que implementam a lógica do jogo da forca.

### `kernel.asm`

Este arquivo contém o código de inicialização de baixo nível (bootstrap) escrito em Assembly (NASM). Ele prepara o ambiente para a execução do código C e fornece funções auxiliares para interação com o hardware.

**Funções e Seções Principais:**
*   **Header Multiboot**: Identifica o kernel como compatível com o padrão Multiboot, permitindo que carregadores de boot como o GRUB o iniciem.
*   `start`: O ponto de entrada (`ENTRY`) do kernel. Ele desabilita as interrupções, configura a pilha (stack) e chama a função `kmain` do arquivo C.
*   `read_port`, `write_port`: Funções globais para ler e escrever em portas de I/O, essenciais para a comunicação com hardware como o PIC e o controlador de teclado.
*   `load_idt`: Carrega o ponteiro da IDT no registrador do processador para ativar a tabela de interrupções.
*   `keyboard_handler`: O handler de interrupção de baixo nível. Ele é chamado diretamente pelo processador quando uma interrupção de teclado ocorre e, por sua vez, chama a função `keyboard_handler_main` em C.

### `linker.ld`

Este é o script do linker, que instrui o montador sobre como organizar o arquivo executável final do kernel.

**Configurações Principais:**
*   `OUTPUT_FORMAT(elf32-i386)`: Define o formato do arquivo de saída como ELF de 32 bits para a arquitetura i386.
*   `ENTRY(start)`: Define o rótulo `start` (do `kernel.asm`) como o ponto de entrada do programa.
*   `SECTIONS`: Define a organização da memória. O código é colocado no endereço base `0x100000`, seguido pelas seções `.text` (código), `.data` (dados inicializados) e `.bss` (dados não inicializados).

## Compilação

Para compilar e rodar o código do bootloader e kernel, basta rodar os comandos abaixo:

```sh
nasm -f elf32 kernel.asm -o kasm.o
gcc -fno-stack-protector -m32 -c kernel.c -o kc.o
ld -m elf_i386 -T linker.ld -o kernel kasm.o kc.o
qemu-system-i386 -kernel kernel

```