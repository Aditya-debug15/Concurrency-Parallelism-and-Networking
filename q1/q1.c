#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#define max_inp_to_entities 100
// color scheme
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

// RED  - STUDENT FILLED PREFERENCE AND EXIT FROM SIMULATION {1. COURSE SELECTED  AND  2. DIDN'T FIND ANYTHING SUITABLE }
// GREEN - STUDENT CHANGED PRIORITY / PREFERENCE  AND WITHDRAWN FROM COURSE
// YELLOW - STUDENT ALLOCATED A SEAT
// BLUE - NUMBER OF SLOTS DECIDED BY COURSE  EXIT BY COURSE
// MAG - ALLOCATION OF TA
// CYN - STARTING OF TUT AND ENDING OF TUT
// BLK - REMOVAL OF LAB

typedef struct students students;
typedef struct c_labs c_labs;
typedef struct courses courses;
int num_students, num_courses, num_c_labs;
struct students
{
    pthread_t thread_obj;
    float calibre;
    int prefer_1;
    int prefer_2;
    int prefer_3;
    int reach_time;
    int curr_stat;
    int thr_id;
    int id;
    pthread_mutex_t mutex;
};
struct courses
{
    pthread_t thread_obj;
    char name[100];
    float interest;
    int max_slot;
    int slot_decided;
    int slot_filled;
    int num_c_labs;
    int c_labs_id[max_inp_to_entities];
    int id;
    int thr_id;
    int status;
    int tut_over;
    pthread_mutex_t mutex;
    pthread_cond_t ready_for_allocation;
    pthread_cond_t tut_is_over;
};
struct c_labs
{
    char name[100];
    int num_TAs;
    int max_allocation;
    int TA_record[max_inp_to_entities];
    int TA_status[max_inp_to_entities];
    int lab_status;
    pthread_mutex_t mutex;
};
students *students_ptr[max_inp_to_entities];
courses *courses_ptr[max_inp_to_entities];
c_labs *c_labs_ptr[max_inp_to_entities];
int get_random_int(int lower, int upper)
{
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}
bool like_or_not(double prob)
{
    int num = get_random_int(0, 100);
    prob *= 100;
    if (num <= prob)
    {
        return true;
    }
    return false;
}
void *init_students(void *ptr)
{
    int id = *((int *)ptr);
    // sleep before arrival
    sleep(students_ptr[id]->reach_time);
    // it has arrived/filled
    printf(BRED "Student %d has filled in preferences for course registration\n" ANSI_RESET, id );
    while (students_ptr[id]->curr_stat != 0)
    {
        if (students_ptr[id]->curr_stat == 1)
        {
            bool change_karna;
            // FIRST PREFERANCE HAI
            int course_id = students_ptr[id]->prefer_1;
            pthread_mutex_lock(&courses_ptr[course_id]->mutex);
            while (courses_ptr[course_id]->slot_decided <= 0 && courses_ptr[course_id]->status <= 0)
            {
                pthread_cond_wait(&courses_ptr[course_id]->ready_for_allocation, &courses_ptr[course_id]->mutex);
                // Equivalent to:
                // pthread_mutex_unlock(&mutexFuel);
                // wait for signal on condFuel
                // pthread_mutex_lock(&mutexFuel);
            }
            if (courses_ptr[course_id]->status == 1)
            {
                printf(BGRN"student %d - Course %s no more, need to change preference \n"ANSI_RESET, id, courses_ptr[course_id]->name);
                // pthread_cond_signal(&courses_ptr[course_id]->ready_for_allocation);
                change_karna = true;
            }
            else
            {
                courses_ptr[course_id]->slot_decided--;
                courses_ptr[course_id]->slot_filled++;
                printf(BYEL"Student %d has been allocated a seat in course %s\n"ANSI_RESET, id, courses_ptr[course_id]->name);
            }
            pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
            if (change_karna)
            {
                printf(BGRN"Student %d has changed current preference from %s (priority 1) to %s (priority 2)\n"ANSI_RESET, id, courses_ptr[course_id]->name, courses_ptr[students_ptr[id]->prefer_2]->name);
                students_ptr[id]->curr_stat = 2;
            }
            else
            {
                pthread_mutex_lock(&courses_ptr[course_id]->mutex);
                while (courses_ptr[course_id]->tut_over == 1)
                {
                    pthread_cond_wait(&courses_ptr[course_id]->tut_is_over, &courses_ptr[course_id]->mutex);
                }
                pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
                // pthread_cond_signal(&courses_ptr[course_id]->tut_is_over);
                //  tut will be over
                //  prob wala kaam
                //  Prob = (interest of the course) x (student’s calibre)
                float prob = courses_ptr[course_id]->interest * students_ptr[id]->calibre;
                bool like = like_or_not(prob);
                if (like)
                {
                    printf(BRED"Student %d has selected course %s permanently\n"ANSI_RESET, id, courses_ptr[course_id]->name);
                    students_ptr[id]->curr_stat = 0;
                }
                else
                {
                    printf(BGRN"Student %d has withdrawn from course %s\n"ANSI_RESET,id,courses_ptr[course_id]->name);
                    printf(BGRN"Student %d has changed current preference from %s (priority 1) to %s (priority 2)\n"ANSI_RESET, id, courses_ptr[course_id]->name, courses_ptr[students_ptr[id]->prefer_2]->name);
                    students_ptr[id]->curr_stat = 2;
                }
            }
        }
        else if (students_ptr[id]->curr_stat == 2)
        {
            bool change_karna;
            // FIRST PREFERANCE HAI
            int course_id = students_ptr[id]->prefer_2;
            pthread_mutex_lock(&courses_ptr[course_id]->mutex);
            while (courses_ptr[course_id]->slot_decided <= 0 && courses_ptr[course_id]->status <= 0)
            {
                pthread_cond_wait(&courses_ptr[course_id]->ready_for_allocation, &courses_ptr[course_id]->mutex);
                // Equivalent to:
                // pthread_mutex_unlock(&mutexFuel);
                // wait for signal on condFuel
                // pthread_mutex_lock(&mutexFuel);
            }
            if (courses_ptr[course_id]->status == 1)
            {
                printf(BGRN"student %d - Course %s no more, need to change preference \n"ANSI_RESET, id, courses_ptr[course_id]->name);
                // pthread_cond_signal(&courses_ptr[course_id]->ready_for_allocation);
                change_karna = true;
            }
            else
            {
                courses_ptr[course_id]->slot_decided--;
                courses_ptr[course_id]->slot_filled++;
                printf(BYEL"Student %d has been allocated a seat in course %s\n"ANSI_RESET, id, courses_ptr[course_id]->name);
            }
            pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
            if (change_karna)
            {
                printf(BGRN"Student %d has changed current preference from %s (priority 2) to %s (priority 3)\n"ANSI_RESET, id, courses_ptr[course_id]->name, courses_ptr[students_ptr[id]->prefer_3]->name);
                students_ptr[id]->curr_stat = 3;
            }
            else
            {
                pthread_mutex_lock(&courses_ptr[course_id]->mutex);
                while (courses_ptr[course_id]->tut_over == 1)
                {
                    pthread_cond_wait(&courses_ptr[course_id]->tut_is_over, &courses_ptr[course_id]->mutex);
                }
                pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
                // tut will be over
                // prob wala kaam
                // Prob = (interest of the course) x (student’s calibre)
                float prob = courses_ptr[course_id]->interest * students_ptr[id]->calibre;
                bool like = like_or_not(prob);
                if (like)
                {
                    printf(BRED"Student %d has selected course %s permanently\n"ANSI_RESET, id, courses_ptr[course_id]->name);
                    students_ptr[id]->curr_stat = 0;
                }
                else
                {
                    printf(BGRN"Student %d has withdrawn from course %s\n"ANSI_RESET,id,courses_ptr[course_id]->name);
                    printf(BGRN"Student %d has changed current preference from %s (priority 2) to %s (priority 3)\n"ANSI_RESET, id, courses_ptr[course_id]->name, courses_ptr[students_ptr[id]->prefer_3]->name);
                    students_ptr[id]->curr_stat = 3;
                }
            }
        }
        else if (students_ptr[id]->curr_stat == 3)
        {
            bool change_karna;
            // FIRST PREFERANCE HAI
            int course_id = students_ptr[id]->prefer_3;
            pthread_mutex_lock(&courses_ptr[course_id]->mutex);
            while (courses_ptr[course_id]->slot_decided <= 0 && courses_ptr[course_id]->status <= 0)
            {
                pthread_cond_wait(&courses_ptr[course_id]->ready_for_allocation, &courses_ptr[course_id]->mutex);
                // Equivalent to:
                // pthread_mutex_unlock(&mutexFuel);
                // wait for signal on condFuel
                // pthread_mutex_lock(&mutexFuel);
            }
            if (courses_ptr[course_id]->status == 1)
            {
                printf(BGRN"student %d - Course %s no more, need to change preference \n"ANSI_RESET, id, courses_ptr[course_id]->name);
                // pthread_cond_signal(&courses_ptr[course_id]->ready_for_allocation);
                change_karna = true;
            }
            else
            {
                courses_ptr[course_id]->slot_decided--;
                courses_ptr[course_id]->slot_filled++;
                printf(BYEL"Student %d has been allocated a seat in course %s\n"ANSI_RESET, id, courses_ptr[course_id]->name);
            }
            pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
            if (change_karna)
            {
                printf(BRED"Student %d couldn’t get any of his preferred courses\n"ANSI_RESET, id);
                students_ptr[id]->curr_stat = 0;
            }
            else
            {
                // sleep ki bajay cond wait karlo ek hi baat hai
                pthread_mutex_lock(&courses_ptr[course_id]->mutex);
                while (courses_ptr[course_id]->tut_over == 1)
                {
                    pthread_cond_wait(&courses_ptr[course_id]->tut_is_over, &courses_ptr[course_id]->mutex);
                }
                pthread_mutex_unlock(&courses_ptr[course_id]->mutex);
                // tut will be over
                // prob wala kaam
                // Prob = (interest of the course) x (student’s calibre)
                float prob = courses_ptr[course_id]->interest * students_ptr[id]->calibre;
                bool like = like_or_not(prob);
                if (like)
                {
                    printf(BRED"Student %d has selected course %s permanently\n"ANSI_RESET, id, courses_ptr[course_id]->name);
                    students_ptr[id]->curr_stat = 0;
                }
                else
                {
                    printf(BGRN"Student %d has withdrawn from course %s\n"ANSI_RESET,id,courses_ptr[course_id]->name);
                    printf(BRED"Student %d couldn’t get any of his preferred courses\n"ANSI_RESET, id);
                    students_ptr[id]->curr_stat = 0;
                }
            }
        }
    }
    return NULL;
}
void *init_courses(void *ptr)
{
    int id = *((int *)ptr);
    // search for TA
    // for every lab need a mutex for comparison purposes
    int golden_number = 0;
    for (int i = 0; i < courses_ptr[id]->num_c_labs; i++)
    {
        int c_lab_id = courses_ptr[id]->c_labs_id[i];
        golden_number += (c_labs_ptr[c_lab_id]->num_TAs);
    }
    while (true)
    {
        bool ta_mila = false;
        int count_ta_over = 0;
        int selected_lab, selected_ta;
        for (int i = 0; i < courses_ptr[id]->num_c_labs; i++)
        {
            int count_ta_over_special = 0;
            int c_lab_id = courses_ptr[id]->c_labs_id[i];
            int silver_number = c_labs_ptr[c_lab_id]->num_TAs;
            for (int j = 0; j < c_labs_ptr[c_lab_id]->num_TAs; j++)
            {
                pthread_mutex_lock(&c_labs_ptr[c_lab_id]->mutex);
                if (c_labs_ptr[c_lab_id]->TA_status[j] == 0 && c_labs_ptr[c_lab_id]->TA_record[j] < c_labs_ptr[c_lab_id]->max_allocation)
                {
                    ta_mila = true;
                    c_labs_ptr[c_lab_id]->TA_status[j] = 1;
                    c_labs_ptr[c_lab_id]->TA_record[j]++;
                    printf(BMAG"TA %d from lab %s has been allocated to course %s for his %d TA ship\n"ANSI_RESET, j, c_labs_ptr[c_lab_id]->name, courses_ptr[id]->name, c_labs_ptr[c_lab_id]->TA_record[j]);
                    selected_lab = c_lab_id;
                    selected_ta = j;
                }
                else if (c_labs_ptr[c_lab_id]->TA_record[j] == c_labs_ptr[c_lab_id]->max_allocation)
                {
                    count_ta_over_special++;
                    count_ta_over++;
                }
                pthread_mutex_unlock(&c_labs_ptr[c_lab_id]->mutex);
                if (ta_mila)
                {
                    break;
                }
            }
            if (ta_mila)
            {
                break;
            }
            pthread_mutex_lock(&c_labs_ptr[c_lab_id]->mutex);
            if (count_ta_over_special == silver_number)
            {
                if (c_labs_ptr[c_lab_id]->lab_status == 0)
                {
                    printf(BBLK"Lab %s no longer has students available for TA ship\n"ANSI_RESET, c_labs_ptr[c_lab_id]->name);
                    c_labs_ptr[c_lab_id]->lab_status = 1;
                }
            }
            pthread_mutex_unlock(&c_labs_ptr[c_lab_id]->mutex);
        }
        if (ta_mila)
        {
            // TA SE KAAM KARWANA HAI
            // D CHOOSE KARNA HAI
            // COND_SIGNAL KARNA HAI
            // WAIT FOR A SEC FOR STUDENT TO JOIN
            pthread_mutex_lock(&courses_ptr[id]->mutex);
            courses_ptr[id]->slot_decided = get_random_int(1, courses_ptr[id]->max_slot);
            printf(BBLU"COURSE %s has decided %d slots\n"ANSI_RESET, courses_ptr[id]->name, courses_ptr[id]->slot_decided);
            courses_ptr[id]->tut_over = 1;
            pthread_mutex_unlock(&courses_ptr[id]->mutex);
            pthread_cond_broadcast(&courses_ptr[id]->ready_for_allocation);

            sleep(2);
            pthread_mutex_lock(&courses_ptr[id]->mutex);
            courses_ptr[id]->slot_decided = 0;
            printf(BCYN"Tutorial has started for course %s has started a tut where slots filled are %d\n"ANSI_RESET, courses_ptr[id]->name, courses_ptr[id]->slot_filled);
            courses_ptr[id]->slot_filled = 0;
            pthread_mutex_unlock(&courses_ptr[id]->mutex);
            sleep(3);
            pthread_mutex_lock(&c_labs_ptr[selected_lab]->mutex);
            c_labs_ptr[selected_lab]->TA_status[selected_ta] = 0;
            printf(BCYN"TA %d from lab %s has completed the tutorial and left the course %s\n"ANSI_RESET,selected_ta,c_labs_ptr[selected_lab]->name,courses_ptr[id]->name);
            pthread_mutex_unlock(&c_labs_ptr[selected_lab]->mutex);
            pthread_mutex_lock(&courses_ptr[id]->mutex);
            courses_ptr[id]->tut_over = 0;
            pthread_mutex_unlock(&courses_ptr[id]->mutex);
            pthread_cond_broadcast(&courses_ptr[id]->tut_is_over);
            sleep(2);
        }
        else if (count_ta_over == golden_number)
        {
            pthread_mutex_lock(&courses_ptr[id]->mutex);
            courses_ptr[id]->status = 1;
            printf(BBLU"Course %s doesn’t have any TA’s eligible and is removed from course offerings\n"ANSI_RESET, courses_ptr[id]->name);
            pthread_mutex_unlock(&courses_ptr[id]->mutex);
            pthread_cond_broadcast(&courses_ptr[id]->ready_for_allocation);
            break;
        }
    }
    return NULL;
}
int main()
{
    scanf("%d %d %d", &num_students, &num_c_labs, &num_courses);
    for (int i = 0; i < num_courses; i++)
    {
        courses_ptr[i] = (courses *)malloc(sizeof(courses));
        courses_ptr[i]->id = i;
        scanf("%s %f %d %d", courses_ptr[i]->name, &courses_ptr[i]->interest, &courses_ptr[i]->max_slot, &courses_ptr[i]->num_c_labs);
        for (int j = 0; j < courses_ptr[i]->num_c_labs; j++)
        {
            scanf(" %d", &courses_ptr[i]->c_labs_id[j]);
        }
        courses_ptr[i]->status = 0;
    }
    for (int i = 0; i < num_students; i++)
    {
        students_ptr[i] = (students *)malloc(sizeof(students));
        students_ptr[i]->id = i;
        scanf(" %f %d %d %d %d", &students_ptr[i]->calibre, &students_ptr[i]->prefer_1, &students_ptr[i]->prefer_2, &students_ptr[i]->prefer_3, &students_ptr[i]->reach_time);
        students_ptr[i]->curr_stat = 1;
    }
    for (int i = 0; i < num_c_labs; i++)
    {
        c_labs_ptr[i] = (c_labs *)malloc(sizeof(c_labs));
        scanf(" %s %d %d", c_labs_ptr[i]->name, &c_labs_ptr[i]->num_TAs, &c_labs_ptr[i]->max_allocation);
        for (int j = 0; j < c_labs_ptr[i]->num_TAs; j++)
        {
            c_labs_ptr[i]->TA_status[j] = 0;
            c_labs_ptr[i]->TA_record[j] = 0;
        }
        c_labs_ptr[i]->lab_status = 0;
        pthread_mutex_init(&(c_labs_ptr[i]->mutex), NULL);
    }
    for (int j = 0; j < num_students; j++)
    {
        pthread_mutex_init(&(students_ptr[j]->mutex), NULL);
        students_ptr[j]->thr_id = pthread_create(&(students_ptr[j]->thread_obj), NULL, init_students, (void *)(&(students_ptr[j]->id)));
    }
    for (int j = 0; j < num_courses; j++)
    {
        pthread_mutex_init(&(courses_ptr[j]->mutex), NULL);
        pthread_cond_init(&(courses_ptr[j]->ready_for_allocation), NULL);
        pthread_cond_init(&(courses_ptr[j]->tut_is_over), NULL);
        courses_ptr[j]->thr_id = pthread_create(&(courses_ptr[j]->thread_obj), NULL, init_courses, (void *)(&(courses_ptr[j]->id)));
    }
    for (int i = 0; i < num_students; i++)
    {
        pthread_join(students_ptr[i]->thread_obj, NULL);
    }
    printf("reached here\n");
    for (int i = 0; i < num_courses; i++)
    {
        pthread_join(courses_ptr[i]->thread_obj, NULL);
    }
}