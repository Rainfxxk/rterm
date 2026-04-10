#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL_main.h>
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

#ifdef __linux__

#include <sys/select.h>

#elif defined(_WIN32)

#include <winsock2.h>

#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif


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
    log_init("log");
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
    if (len <= 0) return;

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

void draw_line(SDL_Renderer *renderer, tline_t *line, font_t *font, int r, int c) {
    arg_t arg = line->str[0].arg;
    char buf[c + 1];
    memset(buf, '\0', c + 1);
    char *ptr = buf;
    int i = 0;
    SDL_Rect rect = {.y = r * font->h, .h = font->h};

    while (i < c) {              // traverse the entire line

        while (line->str[i].ch == '\0' && i < c) { // skip ‘\0' character
            i++;
            continue;
        }
        ptr = buf + i;
        arg = line->str[i].arg;

        // get characters with same argument
        while (line->str[i].ch != '\0' && memcmp(&line->str[i].arg, &arg, sizeof(arg_t)) == 0 && i < c) {
            // printf("%c\n", buf[i]);
            buf[i] = line->str[i].ch;
            i++;
        }

        if (i > c) break;

        // draw the text
        buf[i] = '\0';
        rect.x = (ptr - buf) * font->w;
        rect.w = (i - (ptr - buf)) * font->w;
        draw_text(renderer, ptr, font, arg, &rect);
    }

    return;
}

void draw_cursor(SDL_Renderer *renderer, term_t *term, font_t *font) {
    SDL_Rect cursor_rect = {
        .x = term->cur_x * font->w,
        .y = term->cur_y * font->h,
        .w = font->w,
        .h = font->h
    };

    SDL_SetRenderDrawColor(renderer, RGBA(TERM_CURSOR_BG));
    SDL_RenderFillRect(renderer, &cursor_rect);

    if (term->screen[term->cur_y].str[term->cur_x].ch != '\0' && term->screen[term->cur_y].str[term->cur_x].ch != ' ') {
        char ch[2] = {term->screen[term->cur_y].str[term->cur_x].ch, '\0'};

        SDL_Color fg = {RGBA(TERM_CURSOR_FG)};

        SDL_Surface *text = (SDL_Surface *)CHECK_PTR(
            TTF_RenderText_Blended(font->ttf, ch, fg),
            "Failed to render text: %s",
            SDL_GetError()
        );

        SDL_Texture *texture = (SDL_Texture *)CHECK_PTR(SDL_CreateTextureFromSurface(renderer, text),
            "Failed to create texture from surface: %s",
            SDL_GetError()
        );

        SDL_RenderCopy(renderer, texture, NULL, &cursor_rect);

        SDL_FreeSurface(text);
        SDL_DestroyTexture(texture);
    }
    term_log("draw_cursor: %d, %d\n", term->cur_y, term->cur_x);
}

void draw_term(SDL_Renderer *renderer, term_t *term, font_t *font) {
    pthread_mutex_lock(&term->mutex);
    SDL_Color term_bg = {RGBA(TERM_BACKGROUND)};
    SDL_SetRenderDrawColor(renderer, term_bg.r, term_bg.g, term_bg.b, term_bg.a);
    SDL_RenderClear(renderer);

    for (int j = 0; j < term->r; j++) {
        draw_line(renderer, term->screen + j, font, j, term->c);
    }

    draw_cursor(renderer, term, font);
    SDL_RenderPresent(renderer);
    pthread_mutex_unlock(&term->mutex);
}


void *run_term(void *arg) {
    struct run_term_arg_t *run_term_arg = (struct run_term_arg_t *)CHECK_PTR(arg, "run_term get a NULL"); 
    term_t *term = (term_t *)CHECK_PTR(arg, "term thread get a NULL argument");
    pty_t pty = term->pty;

#ifdef __linux__

    int maxfd = PTY_READ_FD(pty);
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(PTY_READ_FD(pty), &readable);

    while (1) {
        int select_res = select(maxfd + 1, &readable, NULL, NULL, NULL);
        if (select_res == -1) {
            perror("select");
            break;
        }

        if (FD_ISSET(PTY_READ_FD(pty), &readable) && select_res > 0) {
            int len = READ_PTY(term->pty, buffer, BUFFER_SIZE);
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

#elif defined(_WIN32)

    while(1) {
        DWORD avail = 0;
        if (!PeekNamedPipe(PTY_READ_FD(pty), NULL, 0, NULL, &avail, NULL)) return arg;
        if (avail > 0) {
            int len = READ_PTY(term->pty, buffer, BUFFER_SIZE);
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

#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif
}

void *callback(int type, void *arg) {
    SDL_UserEvent user = {.type = SDL_USEREVENT + type, .data1 = arg};
    SDL_Event event = {.user = user};
    SDL_PushEvent(&event);
    return NULL;
}

int resize_num = 0;

void window_handle(SDL_WindowEvent window, SDL_Renderer *renderer, term_t *term, font_t *font) {
    printf("window event:%d\n", window.event);
    switch (window.event) {
        case SDL_WINDOWEVENT_EXPOSED:
            printf("exposed\n");
            break;
        case SDL_WINDOWEVENT_RESIZED:
        // case SDL_WINDOWEVENT_SIZE_CHANGED:
            int w = window.data1, h = window.data2;
            int r = MAX(1, h / font->h);
            int c = MAX(1, w / font->w);
            if (r != term->r || c != term->c) {
                printf("window resized %d: %d %d, %d %d\n", ++resize_num, h, w, r, c);
                term_resize(term, r, c);
            }
            else {
                draw_term(renderer, term, font);
            }
            break;
    }

}

void conbination_key(SDL_Keysym keysym, term_t *term) {
    char str[3];
    int len = 0;
    if (keysym.sym == 1073742048) return;
    if (keysym.mod & KMOD_CTRL) {
        printf("conbination_key: ctrl + %d\n", keysym.sym);
        if (keysym.sym == 64)                    str[0] = 0;
        if (keysym.sym > 90 && keysym.sym < 96)  str[0] = keysym.sym - 64;
        if (keysym.sym > 96 && keysym.sym < 123) str[0] = keysym.sym - 96;
        str[1] = '\0';
        WRITE_PTY(term->pty, str, 2);
    }
}

int main(int argc, char* argv[]) {
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

    pthread_t term_thread;
    CHECK(pthread_create(&term_thread, NULL, run_term, term), "Failed to create term thread");

    int running = 1;
    SDL_Event event;
    SDL_StartTextInput();
    while (running) {
        SDL_WaitEvent(&event);
        #define WRITE_MASTER(str, len) {WRITE_PTY(term->pty, str, len);}
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
                    default: conbination_key(event.key.keysym, term);break;
                }
            );
            CASE(SDL_TEXTINPUT, printf("get text input %s\n", event.text.text); WRITE_MASTER(event.text.text, strlen(event.text.text)););
            CASE(SDL_REDRAW, draw_term(renderer, term, &font););
            CASE(SDL_WINDOWEVENT, window_handle(event.window, renderer, term, &font););
            CASE(SDL_SETTITLE, SDL_SetWindowTitle(win, (const char *)event.user.data1); free(event.user.data1));
        }
    }

    close_font(font);
    SDL_DestroyWindow(win);

    return 0;
}
