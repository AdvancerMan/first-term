                section         .text

                global          _start
                extern          set_zero, read_long, write_long, write_char
_start:
                
                sub             rsp, 2 * LONG_INTEGER_SIZE * 8
                lea             rdi, [rsp + LONG_INTEGER_SIZE * 8]
                mov             rcx, LONG_INTEGER_SIZE
                call            read_long
                mov             rdi, rsp
                call            read_long
                
                lea             rsi, [rsp + LONG_INTEGER_SIZE * 8]
                mov             rbx, rsp
                lea             rsp, [rsp - 2 * LONG_INTEGER_SIZE * 8]
                mov             rdi, rsp
                call            mul_long_long
                
                shl             rcx, 1
                call            write_long

                mov             al, 0x0a
                call            write_char

                jmp             exit

; multiplies long number by a long number
;    rsi -- address of multiplier #1 (unsigned long number)
;    rbx -- address of multiplier #2 (unsigned long number)
;    rcx -- length of long number in qwords
; result:
;    product is written to rdi
;    result length is 2 * rcx
mul_long_long:
                mov             r13, rax  ; multiplier #1 digit
                mov             r14, rdx  ; carry for next digit
                mov             r15, r8   ; counter for outer loop * 8
                push            r9        ; counter for inner loop * 8
                push            r10       ; multiplier #2 digit
                push            r11       ; temporary (carry = 1 or = 0)
                push            r12       ; temporary

                shl             rcx, 1
                call            set_zero
                shl             rcx, 2
                
                xor             r8, r8
.loop1:
                xor             r9, r9
                xor             rdx, rdx
.loop2:
                lea             r12, [rdi + r8]
                mov             r12, [r12 + r9]
                xor             r11, r11
                add             r12, rdx
                adc             r11, 0

                xor             rdx, rdx
                mov             rax, [rsi + r8]
                mov             r10, [rbx + r9]
                mul             r10
                add             rax, r12
                adc             rdx, r11
                lea             r12, [rdi + r8]
                mov             [r12 + r9], rax

                lea             r9, [r9 + 8]
                cmp             r9, rcx
                jne              .loop2

                lea             r12, [rdi + r8]
                mov             [r12 + r9], rdx
                lea             r8, [r8 + 8]
                cmp             r8, rcx
                jne              .loop1

                shr             rcx, 3

                pop             r12
                pop             r11
                pop             r10
                pop             r9
                mov             r8,  r15
                mov             rdx, r14
                mov             rax, r13
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

                section         .rodata
LONG_INTEGER_SIZE: equ          128
