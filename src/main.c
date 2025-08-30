#include <bits/types/struct_timeval.h>
#include <stdio.h>
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


int font_w, font_h;
int cur_x, cur_y;
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

    SDL_Color term_fg = {RGBA(TERM_FOREGROUND)};
    SDL_Color term_bg = {RGBA(TERM_BACKGROUND)};
    SDL_SetRenderDrawColor(renderer, term_bg.r, term_bg.g, term_bg.b, term_bg.a);
    SDL_RenderClear(renderer);
    SDL_Rect rect = {.x = cur_x * font.w, .y = cur_y * font.h, .w = font.w, .h = font.h};
    draw_cursor(renderer, &rect, TERM_FOREGROUND);
    SDL_RenderPresent(renderer);

    int running = 1;
    SDL_Event event;
    SDL_StartTextInput();
    PTY *pty = open_pty();
    term_set_size(pty, TERM_ROW, TERM_COL);
    spawn(pty);

    term_t *term = get_term(TERM_ROW, TERM_COL); 

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

            int cur_x = term->cur_x, cur_y = term->cur_y;
            int line = term_write(term, buffer);
            SDL_Rect rect = {.x = cur_x * font.w, .y = cur_y * font.h, .w = font.w, .h = font.h};
            draw_cursor(renderer, &rect, TERM_BACKGROUND);
            for (int i = 0; i < line; i++) {
                int len = strlen(&term->screen[cur_y][cur_x]);
                if (len == 0) {
                    cur_x = 0;
                    cur_y++;
                    continue;
                }
                SDL_Rect text_rect = {.x = cur_x * font.w, .y = cur_y * font.h, .w = font.w * len, .h = font.h};
                SDL_Surface *text = (SDL_Surface *)CHECK_PTR(
                    TTF_RenderText_Blended(font.ttf, &term->screen[cur_y][cur_x], term_fg),
                    "Failed to render text: %s: %d",
                    SDL_GetError(), len 
                );
                SDL_Texture *texture = (SDL_Texture *)CHECK_PTR(SDL_CreateTextureFromSurface(renderer, text),
                    "Failed to create texture from surface: %s",
                    SDL_GetError()
                );
                SDL_RenderCopy(renderer, texture, NULL, &text_rect);
                cur_x = 0;
                cur_y++;
            }
            rect.x = term->cur_x * font.w;
            rect.y = term->cur_y * font.h;
            draw_cursor(renderer, &rect, TERM_FOREGROUND);
            SDL_RenderPresent(renderer);
        }

        if(SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: running = 0; break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_RETURN:  {
                                write(pty->master, "\n", 1);
                            }
                            break;  
                        case SDLK_BACKSPACE: {
                            }
                            break;
                    }
                    break;
                case SDL_TEXTINPUT:
                    write(pty->master, event.text.text, strlen(event.text.text));
                    break;
            }
        }
    }

    close_font(font);
    SDL_DestroyWindow(win);

    return 0;
}
