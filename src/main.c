#include <SDL2/SDL_timer.h>
#include <pthread.h>
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
#include "config.h"
#include "pty.h"
#include "term.h"
#include "util.h"


#define BUFFER_SIZE 4096
char buffer[BUFFER_SIZE + 1];

typedef struct font_t {
    TTF_Font *ttf;
    int w, h;
} font_t;

void init() {
    CHECK(
        SDL_Init(SDL_INIT_EVERYTHING),
        "Failed to init sdl: %s",
        SDL_GetError()
    );
    CHECK(TTF_Init(), "Failed to init sdl ttf: %s", SDL_GetError());
}

font_t open_font(char *font_file) {
    font_t font;
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

void close_font(font_t font) {
    TTF_CloseFont(font.ttf);
}

void draw_text(SDL_Renderer *renderer, const char *str, font_t *font, arg_t arg, SDL_Rect *rect) {
    int len = strlen(str);

    if (arg.reverse) SWAP(unsigned, arg.bg, arg.fg);

    SDL_SetRenderDrawColor(renderer, RGBA(arg.bg));
    SDL_RenderFillRect(renderer, rect);

    SDL_Color fg = {RGBA(arg.fg)};

    SDL_Surface *text = (SDL_Surface *)CHECK_PTR(
        TTF_RenderText_Blended(font->ttf, str, fg),
        "Failed to render text: %s",
        SDL_GetError()
    );

    SDL_Texture *texture = (SDL_Texture *)CHECK_PTR(SDL_CreateTextureFromSurface(renderer, text),
        "Failed to create texture from surface: %s",
        SDL_GetError()
    );

    SDL_RenderCopy(renderer, texture, NULL, rect);

    SDL_FreeSurface(text);
    SDL_DestroyTexture(texture);
}

void draw_line(SDL_Renderer *renderer, tchar_t *line, font_t *font, int r, int c) {
    arg_t arg = line[0].arg;
    char buf[c + 1];
    char *ptr = buf;
    int i = 0;
    SDL_Rect rect = {.y = r * font->h, .h = font->h};

    if (line[0].ch != '\0') for (; i < c + 1; i++) {
        buf[i] = line[i].ch;
        if (line[i].ch == '\0' || memcmp(&line[i].arg, &arg, sizeof(arg_t)) != 0) {
            buf[i] = '\0';
            rect.x = (ptr - buf) * font->w;
            rect.w = (i - (ptr - buf)) * font->w;
            draw_text(renderer, ptr, font, arg, &rect);
            ptr = buf + i;
            arg = line[i].arg;
            if (line[i].ch == '\0') {
                break;
            }
            buf[i] = line[i].ch;
        }
    }

    rect.x = i * font->w;
    rect.w = (c - i + 1) * font->w;
    log("r: %d i: %d x: %d w %d", r, i, rect.x, rect.w);
    SDL_RenderFillRect(renderer, &rect);
    return;
}

void draw_cursor(SDL_Renderer *renderer, term_t *term, font_t *font) {
    SDL_Rect cursor_rect = {.x = term->cur_x * font->w, .y = term->cur_y * font->h, .w = font->w, .h = font->h};
    SDL_SetRenderDrawColor(renderer, RGBA(TERM_CURSOR_BG));
    if (term->screen[term->cur_y][term->cur_x].ch != '\0') {
        char ch[2] = {term->screen[term->cur_y][term->cur_x].ch, '\0'};
        draw_text(renderer, ch, font, term->arg, &cursor_rect);
    }
    SDL_RenderFillRect(renderer, &cursor_rect);
}

void draw_term(SDL_Renderer *renderer, term_t *term, font_t *font) {
    SDL_Color term_bg = {RGBA(TERM_BACKGROUND)};
    SDL_SetRenderDrawColor(renderer, term_bg.r, term_bg.g, term_bg.b, term_bg.a);
    SDL_RenderClear(renderer);

    for (int j = 0; j < term->r; j++) {
        draw_line(renderer, term->screen[j], font, j, term->c);
    }

    draw_cursor(renderer, term, font);
    SDL_RenderPresent(renderer);
}

struct run_term_arg_t {
    PTY *pty;
    term_t *term;
};

void *run_term(void *arg) {
    struct run_term_arg_t *run_term_arg = (struct run_term_arg_t *)CHECK_PTR(arg, "run_term get a NULL"); 
    PTY *pty = run_term_arg->pty;
    term_t *term = run_term_arg->term;
    int maxfd = pty->master;
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(pty->master, &readable);
    // struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};
    
    while (1) {
        int select_res = select(maxfd + 1, &readable, NULL, NULL, NULL);
        if (select_res == -1) {
            perror("select");
            break;
        }

        if (FD_ISSET(pty->master, &readable) && select_res > 0) {
            int len = read(pty->master, buffer, BUFFER_SIZE);
            if (len <= 0) {
                fprintf(stderr, "Nothing to read from child: ");
                perror(NULL);
                SDL_Event event = {.type = SDL_QUIT};
                SDL_PushEvent(&event);
                break;
            }
            buffer[len] = '\0';

            term_write(term, buffer);
        }
    }

    return arg;
}

void *callback(int type, void *arg) {
    SDL_UserEvent user = {.type = SDL_USEREVENT + type, .data1 = arg};
    SDL_Event event = {.user = user};
    SDL_PushEvent(&event);
    return NULL;
}

void conbination_key(SDL_Keysym keysym, term_t *term) {
    if (keysym.sym) {
    }
}

int main() {
    init();

    font_t font = open_font("./font.ttf");

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

    SDL_SetRenderDrawColor(renderer, RGBA(TERM_BACKGROUND));
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    term_t *term = get_term(TERM_ROW, TERM_COL); 
    term->callback = callback;
    PTY *pty = open_pty();
    term_set_size(pty, TERM_ROW, TERM_COL);
    spawn(pty);

    pthread_t term_thread;
    struct run_term_arg_t arg = {.pty = pty, .term = term};
    CHECK(pthread_create(&term_thread, NULL, run_term, &arg), "Failed to create term thread");

    int running = 1;
    SDL_Event event;
    SDL_StartTextInput();
    while (running) {
        SDL_WaitEvent(&event);
        #define WRITE_MASTER(str, len) {write(pty->master, str, len);}
        char str[2] = {'\0', '\0'};
        switch (event.type) {
            CASE(SDL_QUIT,      running = 0);
            CASE(SDL_KEYDOWN,   switch (event.key.keysym.sym) {
                    CASE(SDLK_BACKSPACE, str[0] = ANSI_BACKSPACE; WRITE_MASTER(str, 1));
                    CASE(SDLK_RETURN,    str[0] = ANSI_RETURN; WRITE_MASTER(str, 1));
                    CASE(SDLK_ESCAPE,    str[0] = ANSI_ESCAPE; WRITE_MASTER(str, 1););
                    CASE(SDLK_TAB,       str[0] = ANSI_TAB; WRITE_MASTER(str, 1));
                    CASE(SDLK_UP,        WRITE_MASTER(ANSI_UP, 3));
                    CASE(SDLK_RIGHT,     WRITE_MASTER(ANSI_RIGHT, 3));
                    CASE(SDLK_LEFT,      WRITE_MASTER(ANSI_LEFT, 3));
                    CASE(SDLK_DOWN,      WRITE_MASTER(ANSI_DOWN, 3));
                    // CASE(SDLK_LCTRL,     str[0] = 3; WRITE_MASTER(str, 1););
                    default: conbination_key(event.key.keysym, term);break;
                }
            );
            CASE(SDL_TEXTINPUT, WRITE_MASTER(event.text.text, strlen(event.text.text)););
            CASE(SDL_REDRAW, draw_term(renderer, term, &font););
            CASE(SDL_SETTITLE, SDL_SetWindowTitle(win, (const char *)event.user.data1); free(event.user.data1));
        }
    }

    close_font(font);
    SDL_DestroyWindow(win);

    return 0;
}
