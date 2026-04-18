.include "hdr.asm"

.section ".rodata1" superfree

tilfont:
.incbin "pvsneslibfont.pic"

palfont:
.incbin "pvsneslibfont.pal"

board_tiles:
.incbin "boardtiles.pic"
board_tiles_end:

board_palette:
.incbin "boardtiles.pal"
board_palette_end:

jump_brr:
.incbin "jump.brr"
jump_brr_end:

eat_brr:
.incbin "eat.brr"
eat_brr_end:

tada_brr:
.incbin "tada.brr"
tada_brr_end:

crash_brr:
.incbin "crash.brr"
crash_brr_end:

.ends
