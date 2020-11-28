#ifndef SETPROCTITLE_H
#define SETPROCTITLE_H

extern "C" {
void spt_init(int argc, char *argv[]);
void setproctitle(const char *fmt, ...);
};
#endif // SETPROCTITLE_H
