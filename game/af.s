	.arch msp430g2553
	.p2align 1,0
	
    .text
jt: 
    .word default ; jt[0] bounce beep
    .word option1 ; jt[1] hit beep
    .word option2 ; jt[2] miss beep
    .word option3 ; jt[3] jingle beep
    
    .global buzzer_next_state
    
buzzer_next_state:
    ; range check on selector (buzzerState)
    cmp &buzzerState, #4 ; buzzerState - 4
    jc default           ; doesn't borrow if buzzerState > 3
    
    ; index into jt
    mov &buzzerState, r12
    add r12, r12         ; r12 = 2 * buzzerState
    mov jt(r12), r0      ; jmp jt[s]
    
    ; switch table options
option1: 
    call #buzzerHit
    jmp end ; break
option2:
    call #buzzerMiss
    jmp end ; break
option3:
    call #buzzerJingle
    jmp end ; break
default:
    call #buzzerBounce
end:
    pop r0 ; return
