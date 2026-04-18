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

.ends
