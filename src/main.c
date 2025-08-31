#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include "pty.h"
#include "term.h"
#include "util.h"

#define MIN(a, b) (a > b? b : a)
#define MAX(a, b) (a > b? a : b)

#define TERM_COL 125
#define TERM_ROW 35
#define FONT_SIZE 15
#define BUFFER_SIZE 4096
#define TERM_BACKGROUND 0x22272eff
#define TERM_FOREGROUND 0xadbac7ff


char buffer[BUFFER_SIZE + 1];

typedef struct Font {
    TTF_Font *ttf;
    int w, h;
} Font;

Font open_font(char *font_file) {
    Font font;
    TTF_Font *ttf_font = (TTF_Font *)CHECK_PTR(
                      TTF_OpenFont(font_file, FONT_SIZE),
                      "Failed to open font: %s",
                      SDL_GetError()
    );
    font.ttf = ttf_font;
    CHECK(
        TTF_SizeText(font.ttf, "A", &font.w, &font.h),
        "Failed to get font size: %s",
        SDL_GetError()
    );

    return font;
}

void close_font(Font font) {
    TTF_CloseFont(font.ttf);
}

void draw_cursor(SDL_Renderer *renderer, SDL_Rect *rect, unsigned rgba) {
    SDL_SetRenderDrawColor(renderer, RGBA(rgba));
    SDL_RenderFillRect(renderer, rect);
}

void draw_term(SDL_Renderer *renderer, term_t *term, Font *font) {
    SDL_Color term_fg = {RGBA(TERM_FOREGROUND)};
    SDL_Color term_bg = {RGBA(TERM_BACKGROUND)};

    SDL_SetRenderDrawColor(renderer, term_bg.r, term_bg.g, term_bg.b, term_bg.a);
    SDL_RenderClear(renderer);

    for (int i = 0; i < term->r; i++) {
        int len = strlen(term->screen[i]);
        if (len == 0) {
            continue;
        }
        SDL_Rect text_rect = {.x = 0, .y = i * font->h, .w = font->w * len, .h = font->h};
        SDL_Surface *text = (SDL_Surface *)CHECK_PTR(
            TTF_RenderText_Blended(font->ttf, term->screen[i], term_fg),
            "Failed to render text: %s",
            SDL_GetError()
        );
        SDL_Texture *texture = (SDL_Texture *)CHECK_PTR(SDL_CreateTextureFromSurface(renderer, text),
            "Failed to create texture from surface: %s",
            SDL_GetError()
        );
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    }
    SDL_Rect cursor_rect = {.x = term->cur_x * font->w, .y = term->cur_y * font->h, .w = font->w, .h = font->h};
    draw_cursor(renderer, &cursor_rect, TERM_FOREGROUND);
    if (term->screen[term->cur_y][term->cur_x] != '\0') {
        char ch[2] = {term->screen[term->cur_y][term->cur_x], '\0'};
        SDL_Surface *text = (SDL_Surface *)CHECK_PTR(
            TTF_RenderText_Blended(font->ttf, ch, term_bg),
            "Failed to render text: %s",
            SDL_GetError()
        );
        SDL_Texture *texture = (SDL_Texture *)CHECK_PTR(SDL_CreateTextureFromSurface(renderer, text),
            "Failed to create texture from surface: %s",
            SDL_GetError()
        );
        SDL_RenderCopy(renderer, texture, NULL, &cursor_rect);
    }
    SDL_RenderPresent(renderer);
}

void init() {
    CHECK(
              SDL_Init(SDL_INIT_EVERYTHING),
              "Failed to init sdl: %s",
              SDL_GetError()
    );
    CHECK(TTF_Init(), "Failed to init sdl ttf: %s", SDL_GetError());
}

int main() {
    init();

    Font font = open_font("./font.ttf");

    SDL_Window *win = (SDL_Window *)CHECK_PTR(
        SDL_CreateWindow("rterm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, font.w * TERM_COL, font.h * TERM_ROW, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS),
        "Failed to create window: %s",
        SDL_GetError()
    );

    SDL_Renderer *renderer = (SDL_Renderer *)CHECK_PTR(
        SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED),
        "Failed to create renderer: %s",
        SDL_GetError()
    );

    term_t *term = get_term(TERM_ROW, TERM_COL); 
    SDL_SetRenderDrawColor(renderer, RGBA(TERM_BACKGROUND));
    SDL_RenderClear(renderer);
    SDL_Rect rect = {.x = term->cur_x * font.w, .y = term->cur_y * font.h, .w = font.w, .h = font.h};
    draw_cursor(renderer, &rect, TERM_FOREGROUND);
    SDL_RenderPresent(renderer);

    int running = 1;
    SDL_Event event;
    SDL_StartTextInput();
    PTY *pty = open_pty();
    term_set_size(pty, TERM_ROW, TERM_COL);
    spawn(pty);


    int maxfd;
    fd_set readable;
    maxfd = pty->master;

    while (running) {
        FD_ZERO(&readable);
        FD_SET(pty->master, &readable);

        struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};
        int select_res = select(maxfd + 1, &readable, NULL, NULL, &timeout);
        if (select_res == -1) {
            perror("select");
            return 1;
        }

        if (FD_ISSET(pty->master, &readable) && select_res > 0) {
            int len = read(pty->master, buffer, BUFFER_SIZE);
            if (len <= 0) {
                fprintf(stderr, "Nothing to read from child: ");
                perror(NULL);
                return 1;
            }
            buffer[len] = '\0';

            term_write(term, buffer);
            draw_term(renderer, term, &font);
        }

        if(SDL_PollEvent(&event)) {
            #define WRITE_MASTER(str, len) {write(pty->master, str, len);}
            char str[2] = {'\0', '\0'};
            switch (event.type) {
                CASE(SDL_QUIT,      running = 0);
                CASE(SDL_KEYDOWN,   switch (event.key.keysym.sym) {
                        CASE(SDLK_BACKSPACE, str[0] = ANSI_BACKSPACE; WRITE_MASTER(str, 1));
                        CASE(SDLK_RETURN,    str[0] = ANSI_RETURN; WRITE_MASTER(str, 1));
                        CASE(SDLK_ESCAPE,    str[0] = ANSI_ESCAPE; WRITE_MASTER(str, 1));
                        CASE(SDLK_TAB,       str[0] = ANSI_TAB; WRITE_MASTER(str, 1));
                        CASE(SDLK_UP,        WRITE_MASTER(ANSI_UP, 3));
                        CASE(SDLK_RIGHT,     WRITE_MASTER(ANSI_RIGHT, 3));
                        CASE(SDLK_LEFT,      WRITE_MASTER(ANSI_LEFT, 3));
                        CASE(SDLK_DOWN,      WRITE_MASTER(ANSI_DOWN, 3));
                    }
                );
                CASE(SDL_TEXTINPUT, WRITE_MASTER(event.text.text, strlen(event.text.text)););
            }
        }
    }

    close_font(font);
    SDL_DestroyWindow(win);

    return 0;
}
