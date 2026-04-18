ifeq ($(strip $(PVSNESLIB_HOME)),)
$(error "Please create an environment variable PVSNESLIB_HOME by following this guide: https://github.com/alekmaul/pvsneslib/wiki/Installation")
endif

AUDIOFILES := res/pollen8.it
export SOUNDBANK := res/soundbank

include ${PVSNESLIB_HOME}/devkitsnes/snes_rules

.SECONDARY: pvsneslibfont.pic pvsneslibfont.pal boardtiles.pic boardtiles.pal
.PHONY: bitmaps all

export ROMNAME := tetris

SMCONVFLAGS := -s -o $(SOUNDBANK) -V -b 5
musics: $(SOUNDBANK).obj

all: musics bitmaps $(ROMNAME).sfc

clean: cleanBuildRes cleanRom cleanGfx cleanAudio

pvsneslibfont.pic: pvsneslibfont.png
	@echo convert font with no tile reduction ... $(notdir $@)
	$(GFXCONV) -s 8 -o 16 -u 16 -p -e 0 -i $<

pvsneslibfont.pal: pvsneslibfont.pic
	@true

boardtiles.pic: boardtiles.bmp
	@echo convert board tiles ... $(notdir $@)
	$(GFXCONV) -s 8 -o 16 -u 16 -p -R -t bmp -i $<

boardtiles.pal: boardtiles.pic
	@true

bitmaps: pvsneslibfont.pic pvsneslibfont.pal boardtiles.pic boardtiles.pal
