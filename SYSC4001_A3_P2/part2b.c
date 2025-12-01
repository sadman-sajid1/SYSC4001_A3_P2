//Sadman Sajid 101303949
//Stefan Martincevic 101295641
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <ctype.h>
#include <time.h>
#include <semaphore.h>

#define EXERCISES 5
#define MAX_EXAMS 20
#define PATH_LEN 20

typedef struct
{
    int exam_index;
    int num_exams;
    int student_number;
    char current_exam[PATH_LEN];
    char exams[MAX_EXAMS][PATH_LEN];
    char rubric[EXERCISES];
    int marked[EXERCISES];
    int finished;
    sem_t mutex;
} shared_t;

static int read_student_from_exam(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        char *p = line;
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p)
        {
            int val = atoi(p);
            fclose(f);
            return val;
        }
    }
    fclose(f);
    return -1;
}

static void load_rubric(const char *path, shared_t *sh)
{
    FILE *f = fopen(path, "r");
    for (int i = 0; i < EXERCISES; i++)
    {
        if (f)
        {
            char line[128];
            if (fgets(line, sizeof(line), f))
            {
                char *c = strchr(line, ',');
                c = (c ? c + 1 : line);
                while (*c && isspace((unsigned char)*c))
                    c++;
                sh->rubric[i] = (*c && *c != '\n') ? *c : 'A' + i;
            }
            else
                sh->rubric[i] = 'A' + i;
        }
        else
            sh->rubric[i] = 'A' + i;
    }
    if (f)
        fclose(f);
}

static void save_rubric(const char *path, shared_t *sh)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    for (int i = 0; i < EXERCISES; i++)
        fprintf(f, "%d, %c\n", i + 1, sh->rubric[i]);
    fclose(f);
}

static int all_marked(shared_t *sh)
{
    for (int i = 0; i < EXERCISES; i++)
        if (!sh->marked[i])
            return 0;
    return 1;
}

static int get_question(shared_t *sh)
{
    for (int i = 0; i < EXERCISES; i++)
        if (!sh->marked[i])
        {
            sh->marked[i] = 1;
            return i;
        }
    return -1;
}

static void update_rubric(shared_t *sh, int id, const char *rubric_path)
{
    for (int i = 0; i < EXERCISES; i++)
    {
        usleep((rand() % 300 + 200) * 1000);
        if (rand() % 4 == 0)
        {
            char old = sh->rubric[i];
            sh->rubric[i] = (old == 'Z') ? 'A' : old + 1;
            printf("TA %d: rubric line %d changed %c -> %c\n", id, i + 1, old, sh->rubric[i]);
            save_rubric(rubric_path, sh);
        }
    }
}

static int load_next(shared_t *sh)
{
    if (sh->exam_index + 1 >= sh->num_exams)
    {
        sh->finished = 1;
        return 0;
    }
    sh->exam_index++;
    snprintf(sh->current_exam, PATH_LEN, "%s", sh->exams[sh->exam_index]);
    int sn = read_student_from_exam(sh->current_exam);
    if (sn < 0)
    {
        sh->finished = 1;
        return 0;
    }
    sh->student_number = sn;
    memset(sh->marked, 0, sizeof(sh->marked));
    return 1;
}

static void ta(shared_t *sh, int id, const char *rubric_path)
{
    srand((unsigned)(time(NULL) ^ getpid() ^ id));
    while (!sh->finished)
    {
        sem_wait(&sh->mutex);
        printf("TA %d: reviewing rubric for student %04d\n", id, sh->student_number);
        update_rubric(sh, id, rubric_path);
        int q = get_question(sh);
        if (q >= 0)
        {
            printf("TA %d: marking student %04d q%d\n", id, sh->student_number, q + 1);
            sem_post(&sh->mutex);
            usleep((rand() % 1000 + 500) * 1000);
            sem_wait(&sh->mutex);
            printf("TA %d: done marking student %04d q%d\n", id, sh->student_number, q + 1);
            sem_post(&sh->mutex);
            continue;
        }
        if (all_marked(sh))
        {
            printf("TA %d: all questions marked for student %04d\n", id, sh->student_number);
            if (!load_next(sh))
            {
                sem_post(&sh->mutex);
                break;
            }
            printf("TA %d: loaded next exam %s (student %04d)\n", id, sh->current_exam, sh->student_number);
        }
        sem_post(&sh->mutex);
        usleep(100000);
    }
    printf("TA %d: exiting\n", id);
    _exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <num_TAs>\n", argv[0]);
        return 1;
    }
    int num_TAs = atoi(argv[1]);
    if (num_TAs < 1)
        num_TAs = 2;
    const char *rubric_path = "rubric.txt";
    shared_t *sh = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(sh, 0, sizeof(shared_t));
    sem_init(&sh->mutex, 1, 1);
    load_rubric(rubric_path, sh);
    int count = 0;
    for (int i = 1; i <= MAX_EXAMS; i++)
    {
        snprintf(sh->exams[count], PATH_LEN, "exams/exam%d.txt", i);
        if (access(sh->exams[count], F_OK) == -1)
            break;
        count++;
    }
    if (count == 0)
    {
        printf("No exams found\n");
        return 1;
    }
    sh->num_exams = count;
    sh->exam_index = 0;
    sh->student_number = read_student_from_exam(sh->exams[0]);
    snprintf(sh->current_exam, PATH_LEN, "%s", sh->exams[0]);
    memset(sh->marked, 0, sizeof(sh->marked));
    printf("Parent: loaded %s (student %04d), %d exams total\n", sh->current_exam, sh->student_number, sh->num_exams);
    for (int i = 0; i < num_TAs; i++)
        if (fork() == 0)
            ta(sh, i + 1, rubric_path);
    for (int i = 0; i < num_TAs; i++)
        wait(NULL);

    sem_destroy(&sh->mutex);
    munmap(sh, sizeof(shared_t));
    printf("Parent: all TAs done\n");
    return 0;
}

