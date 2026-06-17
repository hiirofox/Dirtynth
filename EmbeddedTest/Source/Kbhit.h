#ifndef KBD_H
#define KBD_H

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif

enum KeyCode {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ESC,
    KEY_CHAR  // 普通字符
};

struct KeyEvent {
    int type;   // KeyCode
    int ch;     // 普通字符时使用
};

#ifdef _WIN32

static int kb_kbhit() {
    return _kbhit();
}

static KeyEvent kb_getkey() {
    KeyEvent e;

    int c = _getch();

    if (c == 0 || c == 224) { // 扩展键（方向键）
        int c2 = _getch();
        switch (c2) {
        case 72: e.type = KEY_UP; break;
        case 80: e.type = KEY_DOWN; break;
        case 75: e.type = KEY_LEFT; break;
        case 77: e.type = KEY_RIGHT; break;
        default: e.type = KEY_NONE; break;
        }
        return e;
    }

    e.type = KEY_CHAR;
    e.ch = c;
    return e;
}

#else

static struct termios orig_termios;

static void kb_init() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig_termios);
    t = orig_termios;

    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

static void kb_restore() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

static int kb_kbhit() {
    struct timeval tv = { 0, 0 };
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

static KeyEvent kb_getkey() {
    KeyEvent e;
    unsigned char c;

    if (read(STDIN_FILENO, &c, 1) <= 0) {
        e.type = KEY_NONE;
        return e;
    }

    if (c == 27) { // ESC
        unsigned char seq[2];

        if (read(STDIN_FILENO, &seq[0], 1) <= 0) {
            e.type = KEY_ESC;
            return e;
        }

        if (read(STDIN_FILENO, &seq[1], 1) <= 0) {
            e.type = KEY_ESC;
            return e;
        }

        if (seq[0] == '[') {
            switch (seq[1]) {
            case 'A': e.type = KEY_UP; break;
            case 'B': e.type = KEY_DOWN; break;
            case 'C': e.type = KEY_RIGHT; break;
            case 'D': e.type = KEY_LEFT; break;
            default: e.type = KEY_NONE; break;
            }
        }

        return e;
    }

    e.type = KEY_CHAR;
    e.ch = c;
    return e;
}

#endif

#endif