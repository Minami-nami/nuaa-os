#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Parsed command representation
enum { EXEC = 1, REDIR, PIPE, LIST, BACK };
static char       cwd[256];
static char       computer[256];
static const char whitespace[] = " \t\r\n\v";
static const char symbols[]    = "<|>&;()";
struct passwd    *pw;
#define MAXARGS 10

struct cmd {
    int type;
};

struct execcmd {
    int   type;
    char *argv[MAXARGS], *eargv[MAXARGS];
};

struct redircmd {
    int         type, fd, mode;
    char       *file, *efile;
    struct cmd *cmd;
};

struct pipecmd {
    int         type;
    struct cmd *left, *right;
};

struct listcmd {
    int         type;
    struct cmd *left, *right;
};

struct backcmd {
    int         type;
    struct cmd *cmd;
};

struct cmd *parsecmd(char *);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd) {
    int              p[2];
    struct backcmd  *bcmd;
    struct execcmd  *ecmd;
    struct listcmd  *lcmd;
    struct pipecmd  *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0) exit(1);
    ;

    switch (cmd->type) {
    case EXEC:
        ecmd = (struct execcmd *)cmd;
        if (ecmd->argv[0] == 0) exit(1);
        execvp(ecmd->argv[0], ecmd->argv);
        printf("exec: %s\n", ecmd->argv[0]);
        break;

    case REDIR:
        rcmd = (struct redircmd *)cmd;
        close(rcmd->fd);
        if (open(rcmd->file, rcmd->mode, 0644) < 0) {
            printf("open: %s\n", rcmd->file);
            exit(1);
        }
        runcmd(rcmd->cmd);
        break;

    case LIST:
        lcmd = (struct listcmd *)cmd;
        if (fork() == 0) runcmd(lcmd->left);
        wait(NULL);
        runcmd(lcmd->right);
        break;

    case PIPE:
        pcmd = (struct pipecmd *)cmd;
        assert(pipe(p) >= 0);
#ifdef NDEBUG
        if (pipe(p) >= 0) exit(EXIT_FAILURE);
#endif
        if (fork() == 0) {
            close(STDOUT_FILENO);
            // TODO
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->left);
        }
        if (fork() == 0) {
            close(STDIN_FILENO);
            // TODO
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->right);
        }
        close(p[0]);
        close(p[1]);
        wait(NULL);
        wait(NULL);
        break;

    case BACK:
        bcmd = (struct backcmd *)cmd;
        if (fork() == 0) runcmd(bcmd->cmd);
        break;

    default:
        assert(0);
#ifdef NDEBUG
        if (0) exit(EXIT_FAILURE);
#endif
    }
    exit(EXIT_SUCCESS);
}

int getcmd(char *buf, int nbuf) {
    memset(cwd, 0, sizeof(cwd));
    getcwd(cwd, sizeof(cwd));

    printf("%s@%s:%s$ ", pw->pw_name, computer, cwd);
    fflush(stdout);
    memset(buf, 0, nbuf);

    while (nbuf-- > 1) {
        int nread = read(STDIN_FILENO, buf, 1);
        if (nread <= 0) return -1;
        if (*(buf++) == '\n') break;
    }
    return 0;
}

void start() {
    static char buf[256];
    gethostname(computer, sizeof(computer));
    pw = getpwuid(getuid());
    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0) {
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
            // Chdir must be called by the parent, not the child.
            buf[strlen(buf) - 1] = 0;  // chop \n
            if (chdir(buf + 3) < 0) printf("cd: %s\n", buf + 3);
            continue;
        }
        else if (buf[0] == 'p' && buf[1] == 'w' && buf[2] == 'd' && (buf[3] == 0 || strchr(whitespace, buf[3]))) {
            memset(cwd, 0, sizeof(cwd));
            getcwd(cwd, sizeof(cwd));
            printf("%s\n", cwd);
            continue;
        }
        else if (buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't') {
            if (buf[4] == 0)
                exit(EXIT_SUCCESS);
            else if (!strchr(whitespace, buf[4]))
                continue;
            char *ptr;
            int   retCode = strtol(buf + 5, &ptr, 10);
            exit(retCode);
        }

        if (fork() == 0) runcmd(parsecmd(buf));
        wait(NULL);
    }
    exit(EXIT_SUCCESS);
}

// Constructors

struct cmd *execcmd(void) {
    struct execcmd *cmd;

    cmd       = (struct execcmd *)malloc(sizeof(*cmd));
    cmd->type = EXEC;
    return (struct cmd *)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd) {
    struct redircmd *cmd;

    cmd        = (struct redircmd *)malloc(sizeof(*cmd));
    cmd->type  = REDIR;
    cmd->cmd   = subcmd;
    cmd->file  = file;
    cmd->efile = efile;
    cmd->mode  = mode;
    cmd->fd    = fd;
    return (struct cmd *)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right) {
    struct pipecmd *cmd;

    cmd        = (struct pipecmd *)malloc(sizeof(*cmd));
    cmd->type  = PIPE;
    cmd->left  = left;
    cmd->right = right;
    return (struct cmd *)cmd;
}

struct cmd *listcmd(struct cmd *left, struct cmd *right) {
    struct listcmd *cmd;

    cmd        = (struct listcmd *)malloc(sizeof(*cmd));
    cmd->type  = LIST;
    cmd->left  = left;
    cmd->right = right;
    return (struct cmd *)cmd;
}

struct cmd *backcmd(struct cmd *subcmd) {
    struct backcmd *cmd;

    cmd       = (struct backcmd *)malloc(sizeof(*cmd));
    cmd->type = BACK;
    cmd->cmd  = subcmd;
    return (struct cmd *)cmd;
}

// Parsing

int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s;
    int   ret;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;
    if (q) *q = s;
    ret = *s;
    switch (*s) {
    case 0:
        break;
    case '|':
    case '(':
    case ')':
    case ';':
    case '&':
    case '<':
        s++;
        break;
    case '>':
        s++;
        if (*s == '>') {
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)) s++;
        break;
    }
    if (eq) *eq = s;

    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, const char *toks) {
    char *s;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

void logger(struct cmd *cmd) {
    if (cmd == NULL) return;
    switch (cmd->type) {
    case EXEC:
        printf("EXEC\n");
        break;
    case REDIR:
        printf("REDIR\n");
        break;
    case PIPE:
        printf("PIPE\n");
        break;
    case LIST:
        printf("LIST\n");
        break;
    case BACK:
        printf("BACK\n");
        break;
    default:
        printf("UNKNOWN\n");
        break;
    }
}

struct cmd *parsecmd(char *s) {
    char       *es;
    struct cmd *cmd;

    es  = s + strlen(s);
    cmd = parseline(&s, es);
    peek(&s, es, "");
    assert(s == es);
#ifdef NDEBUG
    if (s == es) exit(EXIT_FAILURE);
#endif

    nulterminate(cmd);
    return cmd;
}

struct cmd *parseline(char **ps, char *es) {
    struct cmd *cmd;

    cmd = parsepipe(ps, es);
    while (peek(ps, es, "&")) {
        gettoken(ps, es, 0, 0);
        cmd = backcmd(cmd);
    }
    if (peek(ps, es, ";")) {
        gettoken(ps, es, 0, 0);
        cmd = listcmd(cmd, parseline(ps, es));
    }
    return cmd;
}

struct cmd *parsepipe(char **ps, char *es) {
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if (peek(ps, es, "|")) {
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es) {
    int   tok;
    char *q, *eq;

    while (peek(ps, es, "<>")) {
        tok = gettoken(ps, es, 0, 0);
        assert(gettoken(ps, es, &q, &eq) == 'a');
#ifdef NDEBUG
        if (gettoken(ps, es, &q, &eq) == 'a') exit(EXIT_FAILURE);
#endif
        switch (tok) {
        case '<':
            cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
            break;
        case '>':
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT | O_TRUNC, 1);
            break;
        case '+':  // >>
            cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT, 1);
            break;
        }
    }
    return cmd;
}

struct cmd *parseblock(char **ps, char *es) {
    struct cmd *cmd;

    assert(peek(ps, es, "("));
#ifdef NDEBUG
    if (peek(ps, es, "(")) exit(EXIT_FAILURE);
#endif
    gettoken(ps, es, 0, 0);
    cmd = parseline(ps, es);
    assert(peek(ps, es, ")"));
#ifdef NDEBUG
    if (peek(ps, es, ")")) exit(EXIT_FAILURE);
#endif
    gettoken(ps, es, 0, 0);
    cmd = parseredirs(cmd, ps, es);
    return cmd;
}

struct cmd *parseexec(char **ps, char *es) {
    char           *q, *eq;
    int             tok, argc;
    struct execcmd *cmd;
    struct cmd     *ret;

    if (peek(ps, es, "(")) return parseblock(ps, es);

    ret = execcmd();
    cmd = (struct execcmd *)ret;

    argc = 0;
    ret  = parseredirs(ret, ps, es);
    while (!peek(ps, es, "|)&;")) {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0) break;
        assert(tok == 'a');
#ifdef NDEBUG
        if (tok == 'a') exit(EXIT_FAILURE);
#endif
        cmd->argv[argc]  = q;
        cmd->eargv[argc] = eq;
        assert(++argc < MAXARGS);
#ifdef NDEBUG
        if (++argc < MAXARGS) exit(EXIT_FAILURE);
#endif
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc]  = 0;
    cmd->eargv[argc] = 0;
    return ret;
}

// NUL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd) {
    int              i;
    struct backcmd  *bcmd;
    struct execcmd  *ecmd;
    struct listcmd  *lcmd;
    struct pipecmd  *pcmd;
    struct redircmd *rcmd;

    if (cmd == 0) return 0;

    switch (cmd->type) {
    case EXEC:
        ecmd = (struct execcmd *)cmd;
        for (i = 0; ecmd->argv[i]; i++) *ecmd->eargv[i] = 0;
        break;

    case REDIR:
        rcmd = (struct redircmd *)cmd;
        nulterminate(rcmd->cmd);
        *rcmd->efile = 0;
        break;

    case PIPE:
        pcmd = (struct pipecmd *)cmd;
        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;

    case LIST:
        lcmd = (struct listcmd *)cmd;
        nulterminate(lcmd->left);
        nulterminate(lcmd->right);
        break;

    case BACK:
        bcmd = (struct backcmd *)cmd;
        nulterminate(bcmd->cmd);
        break;
    }
    return cmd;
}

int main() {
    start();
}