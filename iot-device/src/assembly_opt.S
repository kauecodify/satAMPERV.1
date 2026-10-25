#include "soc/soc.h"

    .text
    .align  4
    .global assembly_filter
    .type   assembly_filter, @function

# float assembly_filter(float input)
# filter
# chamada de função - assembly no Xtensa)
assembly_filter:
    entry   a1, 16          # Prepara o stack frame
    
    # No Xtensa, argumentos float passam no registrador f2
    # retornar o valor de f2 intacto (movregister)
    mov.s   f3, f2          # f3 = f2 (return)
    
    retw
