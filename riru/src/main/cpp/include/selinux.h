#pragma once

void dload_selinux();

extern int (*setsockcreatecon)(const char *con);
extern int (*setfilecon)(const char *, const char *);
