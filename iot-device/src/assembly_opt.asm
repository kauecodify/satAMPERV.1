; Otimizações Assembly para ESP32 (Xtensa LX6)
; Filtro passa-baixa para suavizar leituras do sensor

.section .text
.global assembly_filter

; float assembly_filter(float input)
; Implementa filtro passa-baixa: output = 0.8 * output + 0.2 * input
assembly_filter:
    ; Salvar registradores
    entry sp, 32
    
    ; Carregar valor anterior (armazenado em memória)
    l32r a4, previous_value
    l32i a5, a4, 0
    
    ; Carregar valor de entrada
    l32i a6, a2, 0  ; a2 contém o ponteiro para input
    
    ; Converter para fixed-point (m1000)
    movi a7, 1000
    s32i a7, a3, 0  ; a3 é registrador temporário
    
    ; Calcular: new_value = 0.8 * old + 0.2 * input
    ; Em fixed-point: new = (800 * old + 200 * input) / 1000
    
    ; 800 * old
    movi a8, 800
    mull a9, a8, a5
    
    ; 200 * input
    movi a10, 200
    mull a11, a10, a6
    
    ; Somar
    add a12, a9, a11
    
    ; Dividir por 1000
    movi a13, 1000
    div a14, a12, a13
    
    ; Salvar novo valor
    s32i a14, a4, 0
    
    ; Retornar resultado
    mov a2, a14
    
    ; Restaurar e retornar
    retw

.section .data
previous_value:
    .word 0  ; Valor anterior do filtro
