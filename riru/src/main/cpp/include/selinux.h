#pragma once

void dload_selinux();

extern int (*setsockcreatecon)(const char *con);
extern int (*setfilecon)(const char *, const char *);
extern int (*selinux_check_access)(const char *, const char *, const char *, const char *, void *);