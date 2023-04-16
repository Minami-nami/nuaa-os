#ifndef _PARSE_H
#define _PARSE_H

#include "cmd.h"
#include <stddef.h>

#define MAX_CMDC 10
extern int   parse_cmd(char *line, struct cmd *cmd);
extern int   parse_pipe_cmd(char *line, struct cmd *cmdv);
extern void  dump_pipe_cmd(int cmdc, struct cmd *cmdv);
extern void  dump_cmd(struct cmd *cmd);
extern char *strreplace(char *dest, char *src, const char *oldstr, const char *newstr, size_t len);
#endif
