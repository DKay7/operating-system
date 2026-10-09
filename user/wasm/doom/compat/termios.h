#pragma once

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;

#define NCCS 32

struct termios {
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_cc[NCCS];
};

#define ECHO     0000010
#define ICANON   0000002
#define VTIME    5
#define VMIN     6
#define TCSANOW  0
#define TCIFLUSH 0

static inline int
tcgetattr(int fd, struct termios* t) {
    (void)fd;
    (void)t;
    return 0;
}

static inline int
tcsetattr(int fd, int action, const struct termios* t) {
    (void)fd;
    (void)action;
    (void)t;
    return 0;
}

static inline int
tcflush(int fd, int queue) {
    (void)fd;
    (void)queue;
    return 0;
}
