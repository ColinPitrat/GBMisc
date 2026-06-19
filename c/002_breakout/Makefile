.PHONY: sprites

NAME=breakout

ROM=$(NAME).gbc

PNG_ASSETS = $(wildcard *.png)
ASSETS_SOURCES = $(PNG_ASSETS:.png=.c)
ASSETS_HEADERS = $(PNG_ASSETS:.png=.h)

all: $(ROM)

$(ROM): $(NAME).c $(ASSETS_HEADERS) $(ASSETS_SOURCES)
	lcc -Wm-yc -o $(ROM) $(NAME).c $(ASSETS_SOURCES)

%.c %.h: %.png
	png2asset $< -map -sw 8 -sh 8 -sprite_no_optimize

clean:
	rm -f $(ROM) $(ASSETS_SOURCES) $(ASSETS_HEADERS)
