bits 32 ; Informa ao montador que o código é para arquitetura 32 bits

; Header para configuração do modo multiboot
section .text
		align 4
		dd 0x1BADB002              ; magic (identificador multiboot)
		dd 0x00                    
		dd - (0x1BADB002 + 0x00)   

;  Diretivas para exportação dos símbolos para interação com arquivo C
global start
global keyboard_handler
global read_port
global write_port
global load_idt

; Diretivas para importação dos símbolos em outro arquivo
extern kmain         			; Função do kernel (acessada no arquivo C)
extern keyboard_handler_main 	; Função de leitura dos inputs do teclado (acessada no arquivo C)

;  Funções I/O
read_port:
	mov edx, [esp + 4]
	in al, dx
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

;  Função para carregar a IDT (Interrupt Descriptor Table)
load_idt:
	mov edx, [esp + 4]
	lidt [edx]			; Carrega IDT
	sti                 ; Habilita interrupções (Set Interrupt Flag)
	ret

;  Handler de interrupções do teclado
keyboard_handler:                 
	call    keyboard_handler_main
	iretd   ; Finaliza interação e retorna o controle pra CPU (processo antes da int)

;  Ponto de entrada do kernel
start:
	cli                 ; Bloqueia interrupções
	mov esp, stack_space 
	call kmain          ; Chama função principal (do arquivo C)
	hlt

section .bss			; Define dados não inicializados (tratado no linker)
resb 8192 ; 8KB para pilha
stack_space:			; Aponta para o início da memória reservada