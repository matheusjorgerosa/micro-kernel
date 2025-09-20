// DEFINIÇÕES E VARIÁVEIS GLOBAIS DO KERNEL

#define IDT_SIZE 256
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define IDT_ENTRY_INTERRUPT_GATE 0x8E
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define MAX_PALAVRA_SIZE 15
#define MAX_TENTATIVAS 6 

// Memória de Vídeo e Cursor
unsigned int current_loc = 0;
char *vidptr = (char*)0xb8000;

// Mapeamento de Scancode para ASCII
char keyboard_map[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// VARIÁVEIS E LÓGICA DO JOGO DA FORCA
const char *palavras_disponiveis[] = {
    "PIRAMIDE",
    "FURTIVO",
    "CONCATENACAO"
};

char palavra_secreta[MAX_PALAVRA_SIZE + 1];
char palavra_display[MAX_PALAVRA_SIZE + 1];
char letras_chutadas[26]; // A-Z
int letras_chutadas_count = 0;
int tentativas_restantes = MAX_TENTATIVAS;
int letras_faltando;
char input_char = 0;

// Definição de estruturas 
struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

// Funcoes externas em Assembly (implementadas no kernel.asm)
extern void write_port(unsigned short port, unsigned char data);
extern unsigned char read_port(unsigned short port);
extern void load_idt(unsigned long *idt_ptr);
extern void keyboard_handler();

// Protótipos
void idt_init();
void keyboard_handler_main();
void print_char(char c);
void print_string(const char* str);
void clear_screen();
void to_upper(char *c);
int strlen(const char* str);
void reset_game();
void draw_game_state();
void check_guess(char c);
void kmain(void);

// Exibição e utilidades
void clear_screen() {
    unsigned int j = 0;
    while(j < 80 * 25 * 2) {
        vidptr[j] = ' ';
        vidptr[j+1] = 0x07;
        j = j + 2;
    }
    current_loc = 0;
}

void print_char(char c) {
    if (c == '\n') {
        current_loc = (current_loc / (80 * 2) + 1) * (80 * 2);
    } else if (c == '\b') {
        if (current_loc > 0) {
            current_loc -= 2;
            vidptr[current_loc] = ' ';
            vidptr[current_loc + 1] = 0x07;
        }
    } else {
        vidptr[current_loc++] = c;
        vidptr[current_loc++] = 0x07;
    }
    
    if (current_loc >= 80 * 25 * 2) {
        for (int i = 0; i < 80 * 24 * 2; i++) {
            vidptr[i] = vidptr[i + 80 * 2];
        }
        for (int i = 80 * 24 * 2; i < 80 * 25 * 2; i++) {
            vidptr[i] = ' ';
        }
        current_loc = 80 * 24 * 2;
    }
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str++);
    }
}

int strlen(const char* str) {
    int len = 0;
    while(str[len] != '\0') {
        len++;
    }
    return len;
}

void to_upper(char *c) {
    if (*c >= 'a' && *c <= 'z') {
        *c = *c - 32;
    }
}

// Lógica de interrupções e handlers
void idt_init(void) {
    unsigned long keyboard_address = (unsigned long)keyboard_handler; 
    unsigned long idt_address = (unsigned long)IDT;
    unsigned long idt_ptr[2];

    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = IDT_ENTRY_INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;
    
    // Remapeamento do PIC (necessário para não conflitar com exceções da CPU)
    write_port(0x20 , 0x11); // ICW1 
    write_port(0xA0 , 0x11); // ICW1
    write_port(0x21 , 0x20); // ICW2 (Offset primário para 0x20)
    write_port(0xA1 , 0x28); // ICW2 (Offset secundário para 0x28)
    write_port(0x21 , 0x04); // ICW3 (Mestre tem escravos no IRQ2)
    write_port(0xA1 , 0x02); // ICW3 (Escravo ligado ao IRQ2 do mestre)
    write_port(0x21 , 0x01); // ICW4
    write_port(0xA1 , 0x01); // ICW4

    // Habilita IRQ1 (Teclado) e mascara o resto
    write_port(0x21 , 0xFD); // PIC Mestre
    write_port(0xA1 , 0xFF);  // PIC Escravo

    // Carrega o IDT
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16;

    load_idt(idt_ptr);
}

void keyboard_handler_main(void) {
    unsigned char status = read_port(KEYBOARD_STATUS_PORT);
    
    if (status & 0x01) {
        unsigned char scancode = read_port(KEYBOARD_DATA_PORT);
        
        if (scancode < 128) {
            char c = keyboard_map[scancode];
            
            if (c >= 'a' && c <= 'z') {
                to_upper(&c);
                input_char = c;
            } else if (c == '\n') {
                input_char = '\n';
            } else {
                input_char = 0;
            }
        }
    }
    
    // Envia o EOI
    write_port(0x20, 0x20);
    write_port(0xA0, 0x20);
}

// LÓGICA DO JOGO DA FORCA
void reset_game() {
    static int counter = 0;
    int seed = counter++;
    int palavra_idx = seed % 3; 

    // Copia a palavra secreta e inicializa o display
    int len = strlen(palavras_disponiveis[palavra_idx]);
    int i;
    for(i = 0; i < len; i++) {
        palavra_secreta[i] = palavras_disponiveis[palavra_idx][i];
        palavra_display[i] = '_';
    }
    palavra_secreta[i] = '\0';
    palavra_display[i] = '\0';
    
    letras_faltando = len;
    tentativas_restantes = MAX_TENTATIVAS;
    letras_chutadas_count = 0;
    input_char = 0;

    clear_screen();
}

void draw_game_state() {
    clear_screen();
    print_string("= JOGO DA FORCA DO KERNEL (Tentativas: ");
    print_char(tentativas_restantes + '0');
    print_string(") =\n\n");

    print_string("Palavra: ");
    int i;
    for(i = 0; i < strlen(palavra_display); i++) {
        print_char(palavra_display[i]);
        print_char(' ');
    }
    print_string("\n\n");

    print_string("Chutadas: ");
    for(i = 0; i < letras_chutadas_count; i++) {
        print_char(letras_chutadas[i]);
        print_char(' ');
    }
    print_string("\n\n");

    if (tentativas_restantes > 0 && letras_faltando > 0) {
        print_string("Chute uma letra (A-Z) e pressione Enter para validar: ");
    }
}

void check_guess(char c) {
    int i;

    // 1. Verificar se ja foi chutada
    for(i = 0; i < letras_chutadas_count; i++) {
        if (letras_chutadas[i] == c) {
            draw_game_state();
            print_string("\nEssa letra ja foi chutada! Tente outra.\n");
            return;
        }
    }

    // 2. Adicionar a letra chutada
    letras_chutadas[letras_chutadas_count++] = c;
    letras_chutadas[letras_chutadas_count] = '\0'; 

    // 3. Verificar a palavra
    int hit = 0;
    for(i = 0; i < strlen(palavra_secreta); i++) {
        if (palavra_secreta[i] == c) {
            if (palavra_display[i] == '_') {
                palavra_display[i] = c;
                letras_faltando--;
                hit = 1;
            }
        }
    }

    // 4. Se errou, perder tentativa
    if (!hit) {
        tentativas_restantes--;
    }

    draw_game_state();

    // 5. Verificar Fim de Jogo
    if (letras_faltando == 0) {
        print_string("\nVOCE VENCEU! A palavra era: ");
        print_string(palavra_secreta);
        print_string("\nPressione Enter para jogar de novo.");
    } else if (tentativas_restantes == 0) {
        print_string("\nVOCE PERDEU! A palavra era: ");
        print_string(palavra_secreta);
        print_string("\nPressione Enter para jogar de novo.");
    }
}

// FUNÇÃO PRINCIPAL
void kmain(void) {
    reset_game();
    draw_game_state();
    idt_init();
    
    while (1) {
        // Logica para processar o caractere digitado (input_char)
        if (input_char) {
            if (letras_faltando == 0 || tentativas_restantes == 0) {
                // Fim de jogo: espera por Enter para resetar
                if (input_char == '\n') {
                    reset_game();
                    draw_game_state();
                }
            } else if (input_char >= 'A' && input_char <= 'Z') {
                check_guess(input_char);
            }
            
            // Limpa o buffer de entrada para esperar a proxima interrupcao
            input_char = 0; 
        }
    }
}