#include <dirent.h>
#include <libgen.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINEBUF 256
#define MAXPATHLEN 256
#define THREAD_NUM 16
#define QUEUE_SIZE 16

int start = 0, end = 0;

struct THREAD_FIND_ARG {
    char  path[MAXPATHLEN];
    char *target;
};

struct THREAD_FIND_ARG queue[QUEUE_SIZE];
pthread_mutex_t        mutex;
sem_t                  empty, full;

void put_task(char *path, char *target) {
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);

    queue[end].target = target;
    if (path == NULL)
        queue[end].path[0] = '\0';
    else
        strcpy(queue[end].path, path);

    end = (end + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&mutex);
    sem_post(&full);
}

struct THREAD_FIND_ARG *get_task() {

    struct THREAD_FIND_ARG *arg = &queue[start];
    start                       = (start + 1) % QUEUE_SIZE;

    return arg;
}

void find_file(char *path, char *target) {
    FILE *file = fopen(path, "r");

    char line[LINEBUF];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, target)) printf("%s: %s", path, line);
    }

    fclose(file);
}

void find_dir(char *path, char *target) {
    DIR           *dir = opendir(path);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        if (entry->d_type != DT_DIR && entry->d_type != DT_REG) continue;

        char *c_dir = malloc(MAXPATHLEN * sizeof(char));
        strcpy(c_dir, path);
        strcat(c_dir, "/");
        strcat(c_dir, entry->d_name);

        if (entry->d_type == DT_DIR) find_dir(c_dir, target);

        if (entry->d_type == DT_REG) put_task(c_dir, target);

        free(c_dir);
    }
    closedir(dir);
}

void *thread_func(void *arg) {
    struct THREAD_FIND_ARG *Cur;

    while (1) {
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        Cur          = get_task();
        char *path   = strdup(Cur->path);
        char *target = Cur->target;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        if (target == NULL) return NULL;

        find_file(path, target);

        free(path);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        puts("Usage: sfind file string");
        return 0;
    }

    char *path   = argv[1];
    char *string = argv[2];

    struct stat info;
#ifdef DEBUG
    char Root_Dir[MAXPATHLEN];
    printf("path: %s\n", getcwd(Root_Dir, MAXPATHLEN));
#endif
    if ((stat(path, &info)) == -1) {
        perror("stat");
        return 1;
    }

    char *c_dir = strdup(path), *dir;
    chdir((dir = dirname(c_dir)));

    char *c_name = strdup(path), *name;
    name         = basename(c_name);

    pthread_mutex_init(&mutex, NULL);
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, QUEUE_SIZE);
    pthread_t threads[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; ++i) pthread_create(&(threads[i]), NULL, thread_func, NULL);

    if (S_ISDIR(info.st_mode))
        find_dir(name, string);
    else
        put_task(name, string);

    for (int i = 0; i < THREAD_NUM; ++i) put_task(NULL, NULL);

    for (int i = 0; i < THREAD_NUM; ++i) pthread_join(threads[i], NULL);

    free(c_dir);
    free(c_name);
    return 0;
}
