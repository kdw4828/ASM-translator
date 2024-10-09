.align 2
.data
nl: .asciiz "\n"
.align 2
.text
_println:
    li $v0, 1
    lw $a0, 0($sp)
    syscall
    li $v0, 4
    la $a0, nl
    syscall
    jr $ra


# Enter function
.align 2
.text
_main:
    la $sp, -8($sp) # allocate space for old $fp and $ra
    sw $fp, 4($sp) # save old $fp
    sw $ra, 0($sp) # save return address
    la $fp, 0($sp) # set up frame pointer
    la $sp, -96($sp) # allocate stack frame


    # ASSG _temp0 = 123
    li $t0, 123
    sw $t0, -8($fp)


    # ASSG _temp1 = 456
    li $t0, 456
    sw $t0, -12($fp)

    # PLUS temp0 + temp1
    lw $t0, -8($fp)
    lw $t1, -12($fp)
    add $t2, $t0, $t1
    sw $t2, -16($fp)

    # ASSG _x = _temp2
    lw $t0, -16($fp)
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)


    # ASSG _temp3 = 123
    li $t0, 123
    sw $t0, -20($fp)


    # ASSG _temp4 = 456
    li $t0, 456
    sw $t0, -24($fp)

    # SUB temp3 - temp4
    lw $t0, -20($fp)
    lw $t1, -24($fp)
    sub $t2, $t0, $t1
    sw $t2, -28($fp)

    # ASSG _x = _temp5
    lw $t0, -28($fp)
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)


    # ASSG _temp6 = 123
    li $t0, 123
    sw $t0, -32($fp)


    # ASSG _temp7 = 3
    li $t0, 3
    sw $t0, -36($fp)

    # MULT temp6 * temp7
    lw $t0, -32($fp)
    lw $t1, -36($fp)
    mul $t2, $t0, $t1
    sw $t2, -40($fp)

    # ASSG _x = _temp8
    lw $t0, -40($fp)
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)


    # ASSG _temp9 = 123
    li $t0, 123
    sw $t0, -44($fp)


    # ASSG _temp10 = 3
    li $t0, 3
    sw $t0, -48($fp)

    # DIV temp9 / temp10
    lw $t0, -44($fp)
    lw $t1, -48($fp)
    div $t2, $t0, $t1
    sw $t2, -52($fp)

    # ASSG _x = _temp11
    lw $t0, -52($fp)
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)

    # ASSG _temp12 = 123
    li $t0, 123
    sw $t0, -56($fp)

    # ASSG _x = _temp12
    li $t0, 123
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)


    # ASSG _temp13 = 123
    li $t0, 123
    sw $t0, -60($fp)

    # Unary temp13
    lw $t0, -60($fp)
    neg $t1, $t0
    sw $t1, -64($fp)

    # ASSG _x = _temp14
    lw $t0, -64($fp)
    sw $t0, -4($fp)

    # PARAM _x
    lw $t0, -4($fp)
    la $sp, -4($sp)
    sw $t0, 0($sp)

    # CALL
    jal _println
    la $sp 4($sp)

    li $t0, 0
    sw $t0, -4($fp)

# RETURN
    lw $v0, -4($fp)
    la $sp, 0($fp)
    lw $ra, 0($sp)
    lw $fp, 4($sp)
    la $sp, 8($sp)
    jr $ra

.align 2
.text
main:
    j _main

