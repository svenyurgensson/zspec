; Taken from https://spectrumcomputing.co.uk/forums/viewtopic.php?p=77605
; just for illustrative purposes
; compile: sjasmplus --sld=zx_screen_proc.sld  --fullpath zx_screen_proc.asm

    DEVICE ZXSPECTRUM128
    SLDOPT COMMENT WPMEM, LOGPOINT, ASSERTION

    ORG 0xC000

; in:
;   hl - screen address
; out:
;   hl - screen address, one pixel lower
;   flag C if address is out of screen
down_hl:
        inc h
        ld a,h
        and $07
        ret nz

        ld a,h
        sub $08
        ld h,a

        ld a,l
        add a,$20
        ld l,a
        ret nc

        ld a,h
        add a,$08
        ld h,a

        and $18
        cp $18

        ccf
        ret

; brief: Converts pixel coord (x,y) to screen address
; in:
;   H: y(0..192), L: x(0..255)
; out:
;   DE at corresponding byte in the screen
xy2screen:
        LD A,H
        RRA
        RRA
        RRA
        AND 24
        OR 64
        LD D,A
        LD A,H
        AND 7
        OR D
        LD D,A
        LD A,H
        RLA
        RLA
        AND 224
        LD E,A
        LD A,L
        RRA
        RRA
        RRA
        AND 31
        OR E
        LD E,A
        RET

; brief: Converts screen char coord (x,y) to screen address
; in:
;   D: y(0..23), E: x(0..31)
; out:
;   DE at corresponding byte in the screen
xy2char:
        ld a,d
        rrca
        rrca
        rrca
        and 224
        or e
        ld e,a	;low byte sorted
        ld a,d
        and 24
        or 64
        ld d,a
        ret

    SAVEBIN 'examples/zs_screen_proc.bin', down_hl