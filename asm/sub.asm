                section         .text

                global          _start
                extern          read_long, write_long, write_char
_start:
                
                sub             rsp, 2 * LONG_INTEGER_SIZE * 8
                lea             rdi, [rsp + LONG_INTEGER_SIZE * 8]
                mov             rcx, LONG_INTEGER_SIZE
                call            read_long
                mov             rdi, rsp
                call            read_long
                
                mov             rsi, rsp
                lea             rdi, [rsp + LONG_INTEGER_SIZE * 8]
                call            sub_long_long
                
                call            write_long

                mov             al, 0x0a
                call            write_char

                jmp             exit

; subtracts one unsigned long number from another unsigned long number
;    rdi -- address of minuend (unsigned long number)
;    rsi -- address of subtrahend (unsigned long number)
;    rcx -- length of long numbers in qwords
; result:
;    result is written to rdi
sub_long_long:
                push            rdi
                push            rsi
                push            rcx
                push            rax

                clc
.loop:
                mov             rax, [rsi]
                lea             rsi, [rsi + 8]
                sbb             [rdi], rax
                lea             rdi, [rdi + 8]
                dec             rcx
                jnz             .loop

                pop             rax
                pop             rcx
                pop             rsi
                pop             rdi
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

                section         .rodata
LONG_INTEGER_SIZE: equ          128

