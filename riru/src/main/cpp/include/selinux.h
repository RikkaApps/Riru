#pragma once

void dload_selinux();

extern int (*setsockcreatecon)(const char *con);