                section         .text

                global          _start
                extern          set_zero, read_long, write_long, write_char
_start:
                
                sub             rsp, 2 * long_integer_size * 8
                lea             rdi, [rsp + long_integer_size * 8]
                mov             rcx, long_integer_size
                call            read_long
                mov             rdi, rsp
                call            read_long
                
                lea             rsi, [rsp + long_integer_size * 8]
                mov             rbx, rsp
                lea             rsp, [rsp - 2 * long_integer_size * 8]
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
                push            rax  ; copy of [r12] for mul
                push            rdx  ; higher bits of 32-bit multiply --> carry for next digit
                push            r8   ; carry for case when [r14] + rdx overflows
                push            r9   ; counter for outer loop
                push            r10  ; counter for inner loop
                push            r12  ; pointer on multiplier #2 for inner loop 
                push            rsi  ; pointer on multiplier #1 for outer loop
                push            r13  ; copy of [rsi] for mul
                push            rdi  ; pointer on result for outer loop
                push            r14  ; pointer on result for inner loop
                push            r15  ; temporary for adding to [r14]

                shl             rcx, 1
                call            set_zero
                shr             rcx, 1
                
                mov             r9, rcx
.loop1:
                mov             r10, rcx
                mov             r12, rbx
                mov             r14, rdi
                
                xor             rdx, rdx
                clc
.loop2:
                mov             r15, [r14]
                xor             r8, r8
                add             r15, rdx
                adc             r8, 0
                xor             rdx, rdx

                mov             rax, [r12]
                mov             r13, [rsi]
                mul             r13
                add             r15, rax
                adc             rdx, r8
                mov             [r14], r15

                lea             r12, [r12 + 8]
                lea             r14, [r14 + 8]
                dec             r10
                jnz             .loop2

                ; no overflow here because [r14] = 0
                add             [r14], rdx
                lea             rsi, [rsi + 8]
                lea             rdi, [rdi + 8]
                dec             r9
                jnz             .loop1
                
                pop             r15
                pop             r14
                pop             rdi
                pop             r13
                pop             rsi
                pop             r12
                pop             r10
                pop             r9
                pop             r8
                pop             rdx
                pop             rax
                ret

exit:
                mov             rax, 60
                xor             rdi, rdi
                syscall

                section         .rodata
long_integer_size: equ          128
invalid_char_msg:
                db              "Invalid character: "
invalid_char_msg_size: equ             $ - invalid_char_msg
