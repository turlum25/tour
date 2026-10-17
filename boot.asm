; TOUR (TOUR Offends UNIX Rules) Bootloader
; By Turlum25, October 2026
; Plz wen eta better bootloader? This is full of shit. I should have used OSDev guide. 

; Please don't read this code. I swear.
; You will try to bleach your eyes. Please. This bullshit looks like it was vibecoded

; drtuihgfdhfkdjhfduiguidrhguifdhuirhg


[bits 16]
[org 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov di, 0x1000
    xor ebx, ebx
    xor ebp, ebp
    mov edx, 0x534D4150
    mov eax, 0xE820
    mov [di + 20], dword 1
    mov ecx, 24
    int 0x15
    jc .fallback
    cmp eax, edx
    jne .fallback

.e820_loop:
    cmp cx, 20
    jb .skip_entry
    cmp cx, 24
    jb .check_type
    test byte [di + 20], 1
    je .skip_entry

.check_type:
    cmp dword [di + 16], 1      ; type 1: usabel ram
                                ; IS THAT RAM?!?! GET HIM
    jne .skip_entry

    mov eax, [di + 8]           ; length low bytes
    mov edx, [di + 12]          ; len high bytes
    shr eax, 10                 ; the bytes are transgender. they identify as kilobytes now
    shl edx, 22
    or eax, edx
    add ebp, eax                ; add to kb total
    ; Re: I somehow fixed the ram issue. I dont know how

.skip_entry:
    test ebx, ebx
    je .e820_done

    add di, 24

    mov edx, 0x534D4150
    mov eax, 0xE820
    mov [di + 20], dword 1
    mov ecx, 24
    int 0x15
    jc .fallback
    jmp .e820_loop

.e820_done:
    mov eax, ebp
    jmp .save_memory

.fallback:
    xor ecx, ecx
    xor edx, edx
    mov ax, 0xE801
    int 0x15
    jc .error     
    cmp ah, 0x86 
    je .error

    and ecx, 0xFFFF   
    and edx, 0xFFFF

    shl edx, 6

    add ecx, edx
    add ecx, 1024     
    mov eax, ecx
    jmp .save_memory



.error:
    xor eax, eax

.save_memory:
    mov dword [0x7E00], eax
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    mov si, msg
    call print_string_16

    in al, 0x92
    and al, 0xFE
    or al, 0x02
    out 0x92, al

    mov ax, 0x2401
    int 0x15

    mov dl, [boot_drive]
    mov bx, 0x55AA
    mov ah, 0x41
    int 0x13
    jc .chs_read
    cmp bx, 0xAA55
    jne .chs_read
    test cx, 1
    jz .chs_read

    mov si, disk_address_packet
    mov dl, [boot_drive]
    call bios_read_sectors
    jc .chs_read
    jmp .disk_done

.chs_read:
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov cl, 2
    mov ch, 0
    mov dh, 0
    mov di, 64     

.read_sector_loop:
    push di
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jnc .sector_ok

    xor ax, ax
    int 0x13
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc disk_crash

.sector_ok:
    pop di
    
    mov ax, es
    add ax, 0x0020
    mov es, ax
    
    dec di
    cmp di, 0
    je .disk_done

    inc cl
    
    mov al, [boot_drive]
    cmp al, 0x80
    jae .hd_limits
    cmp cl, 19
    jl .read_sector_loop
    jmp .track_step

.hd_limits:
    cmp cl, 64
    jl .read_sector_loop

.track_step:
    mov cl, 1
    inc dh
    cmp dh, 2
    jl .read_sector_loop

    mov dh, 0
    inc ch
    jmp .read_sector_loop

.disk_done:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp 0x08:.pm_entry

[bits 32]
.pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x10000

[bits 16]
disk_crash:
    jmp $

print_string_16:
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    ret

bios_read_sectors:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov ah, 0x42
    int 0x13
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

msg db "Booted", 0x0D, 0x0A, 0
boot_drive db 0

align 4
disk_address_packet:
    db 0x10, 0
    dw 64               
    dw 0x0000                   
    dw 0x1000                   
    dq 1

align 8
gdt_start:
gdt_null:
    dd 0x0, 0x0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510-($-$$) db 0
dw 0xAA55
