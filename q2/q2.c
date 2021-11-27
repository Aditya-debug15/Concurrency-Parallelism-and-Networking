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
typedef struct person person;
typedef struct num_group num_group;
typedef struct match match;
typedef struct match_thread match_thread;
int home_capacity;
int neutral_capacity;
int away_capacity;
int spectating_time;
int num_groups;
struct num_group
{
    int id;
    int num_person;
};
struct person
{
    pthread_t thread_obj;
    char name[100];
    char fan;
    int reach_time;
    int pat_time;
    int num_goals;
    int curr_stat;
    int thr_id;
    int id;
    pthread_mutex_t mutex;
};
struct match
{
    char team; // TWO VALUES H OR A
    int g_time;
    double prob; // between 0 to 1
};
struct match_thread
{
    pthread_t thread_obj;
    int thr_id;
    pthread_mutex_t mutex;
};
int current_time;
int goal_current_time;
person *spectator_ptr[max_inp_to_entities];
num_group *num_group_ptr[max_inp_to_entities];
match *match_ptr[max_inp_to_entities];
match_thread mt;
int home_available, away_available, neutral_available;
pthread_mutex_t time_ke_liye;
pthread_mutex_t goal_time_ke_liye;
pthread_mutex_t lock_tickets;
pthread_cond_t  for_stay;
pthread_mutex_t simulation;
sem_t combi_home_neutral;
sem_t combi_home_away_neutral;
sem_t away_tickets;
int total_person;
int goal_chances;
int home_goals;
int away_goals;
int st_count;
int get_random_int(int lower, int upper)
{
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}
bool goal_or_not(double prob)
{
    int num = get_random_int(0, 100);
    prob *= 100;
    if (num <= prob)
    {
        return true;
    }
    return false;
}
void *init_match(void *ptr)
{
    for (int i = 0; i < goal_chances; i++)
    {
        int goaltime = match_ptr[i]->g_time;
        pthread_mutex_lock(&time_ke_liye);
        int curi_time = current_time;
        pthread_mutex_unlock(&time_ke_liye);
        if (goaltime > curi_time)
        {
            sleep(goaltime - curi_time);
        }
        bool result = goal_or_not(match_ptr[i]->prob);
        if (result)
        {
            pthread_mutex_lock(&goal_time_ke_liye);
            if (match_ptr[i]->team == 'H')
            {
                home_goals++;
            }
            else
            {
                away_goals++;
            }
            pthread_mutex_unlock(&goal_time_ke_liye);
            pthread_cond_broadcast(&for_stay);
            printf("%c has scored the goal at t=%d\n", match_ptr[i]->team, current_time);
        }
        else
        {
            printf("%c missed the goal at t=%d\n", match_ptr[i]->team, current_time);
        }
    }
    pthread_mutex_lock(&simulation);
    st_count++;
    pthread_mutex_unlock(&simulation);
    return NULL;
}
void *init_spectator(void *ptr)
{
    int id = *((int *)ptr);
    int s;
    int arrival_time = spectator_ptr[id]->reach_time;
    pthread_mutex_lock(&time_ke_liye);
    int curi_time = current_time;
    pthread_mutex_unlock(&time_ke_liye);
    if (arrival_time > curi_time)
    {
        sleep(arrival_time - curi_time);
    }
    printf("%s has arrived at %d \n", spectator_ptr[id]->name, current_time);
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        return NULL;
    }
    // need to check the nature of the fan
    ts.tv_sec += spectator_ptr[id]->pat_time;
    if (spectator_ptr[id]->fan == 'H')
    {
        while ((s = sem_timedwait(&combi_home_neutral, &ts)) == -1 && errno == EINTR)
            continue; /* Restart if interrupted by handler */
        /* Check what happened */
        if (s == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf("%s leaving beacuse of low patience at t=%d \n", spectator_ptr[id]->name, current_time);
            }
            else
                perror("sem_timedwait");
        }
        else
        {
            int case_identifier;
            pthread_mutex_lock(&lock_tickets);
            if (home_available > 0)
            {
                case_identifier = 1;
                home_available--;
                printf("%s got a seat at H at t=%d \n", spectator_ptr[id]->name, current_time);
            }
            else
            {
                case_identifier = 2;
                neutral_available--;
                printf("%s got a seat at N at t=%d \n", spectator_ptr[id]->name, current_time);
            }
            if (case_identifier == 1 || case_identifier == 2)
            {
                sem_wait(&combi_home_away_neutral);
            }
            pthread_mutex_unlock(&lock_tickets);
            int cur_ti;
            pthread_mutex_lock(&goal_time_ke_liye);
            cur_ti = goal_current_time;
            cur_ti += spectating_time;
            pthread_mutex_unlock(&goal_time_ke_liye);

            pthread_mutex_lock(&goal_time_ke_liye);
            while (cur_ti > current_time && away_goals < spectator_ptr[id]->num_goals)
            {
                pthread_cond_wait(&for_stay, &goal_time_ke_liye);
            }
            pthread_mutex_unlock(&goal_time_ke_liye);
            // sleep(spectating_time);
            if (away_goals >= spectator_ptr[id]->num_goals)
            {
                printf("%s leaving because of poor performance of his team\n", spectator_ptr[id]->name);
            }
            else
            {
                printf("%s leaving beacuse he needs to complete his assignment at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            pthread_mutex_lock(&lock_tickets);
            if (case_identifier == 1)
            {
                home_available++;
            }
            else if (case_identifier == 2)
            {
                neutral_available++;
            }
            if (case_identifier == 1 || case_identifier == 2)
            {
                sem_post(&combi_home_away_neutral);
            }
            sem_post(&combi_home_neutral);
            pthread_mutex_unlock(&lock_tickets);
        }
    }
    if (spectator_ptr[id]->fan == 'A')
    {
        while ((s = sem_timedwait(&away_tickets, &ts)) == -1 && errno == EINTR)
            continue; /* Restart if interrupted by handler */
        /* Check what happened */
        if (s == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf("%s leaving beacuse of low patience at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            else
                perror("sem_timedwait");
        }
        else
        {
            pthread_mutex_lock(&lock_tickets);
            if (away_available > 0)
            {
                away_available--;
                printf("%s got a seat at A at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            sem_wait(&combi_home_away_neutral);
            pthread_mutex_unlock(&lock_tickets);
            int cur_ti;
            pthread_mutex_lock(&time_ke_liye);
            cur_ti = current_time;
            cur_ti += spectating_time;
            pthread_mutex_unlock(&time_ke_liye);
            pthread_mutex_lock(&goal_time_ke_liye);
            while (cur_ti > current_time && home_goals < spectator_ptr[id]->num_goals)
            {
                pthread_cond_wait(&for_stay, &goal_time_ke_liye);
            }
            pthread_mutex_unlock(&goal_time_ke_liye);
            // sleep(spectating_time);
            if (home_goals >= spectator_ptr[id]->num_goals)
            {
                printf("%s leaving because of poor performance of his team\n", spectator_ptr[id]->name);
            }
            else
            {
                printf("%s leaving beacuse he needs to complete his assignment at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            // sleep(spectating_time);
            // printf("%s leaving beacuse he needs to complete his assignment at t=%d\n", spectator_ptr[id]->name, current_time);
            pthread_mutex_lock(&lock_tickets);
            away_available++;
            sem_post(&combi_home_away_neutral);
            sem_post(&away_tickets);
            pthread_mutex_unlock(&lock_tickets);
        }
    }
    if (spectator_ptr[id]->fan == 'N')
    {
        while ((s = sem_timedwait(&combi_home_away_neutral, &ts)) == -1 && errno == EINTR)
            continue; /* Restart if interrupted by handler */
        /* Check what happened */
        if (s == -1)
        {
            if (errno == ETIMEDOUT)
            {
                printf("%s leaving beacuse of low patience at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            else
                perror("sem_timedwait");
        }
        else
        {
            int case_identifier;
            pthread_mutex_lock(&lock_tickets);
            if (home_available > 0)
            {
                case_identifier = 1;
                home_available--;
                printf("%s got a seat at H at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            else if (neutral_available > 0)
            {
                case_identifier = 2;
                neutral_available--;
                printf("%s got a seat at N at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            else
            {
                case_identifier = 3;
                away_available--;
                printf("%s got a seat at A at t=%d\n", spectator_ptr[id]->name, current_time);
            }
            if (case_identifier == 1 || case_identifier == 2)
            {
                sem_wait(&combi_home_neutral);
            }
            pthread_mutex_unlock(&lock_tickets);
            sleep(spectating_time);
            printf("%s leaving beacuse he needs to complete his assignment at t=%d\n", spectator_ptr[id]->name, current_time);
            pthread_mutex_lock(&lock_tickets);
            if (case_identifier == 1)
            {
                home_available++;
            }
            else if (case_identifier == 2)
            {
                neutral_available++;
            }
            if (case_identifier == 1 || case_identifier == 2)
            {
                sem_post(&combi_home_neutral);
            }
            sem_post(&combi_home_away_neutral);
            pthread_mutex_unlock(&lock_tickets);
        }
    }
    pthread_mutex_lock(&simulation);
    st_count++;
    pthread_mutex_unlock(&simulation);
    return NULL;
}
void semaphore_init()
{
    sem_init(&away_tickets, 0, away_capacity);
    sem_init(&combi_home_neutral, 0, (home_capacity + neutral_capacity));
    sem_init(&combi_home_away_neutral, 0, (home_capacity + neutral_capacity + away_capacity));
    pthread_mutex_init(&lock_tickets,NULL);
    pthread_mutex_init(&time_ke_liye, NULL);
    pthread_mutex_init(&goal_time_ke_liye, NULL);
    pthread_cond_init(&for_stay,NULL);
    pthread_mutex_init(&simulation,NULL);
    home_available = home_capacity;
    neutral_available = neutral_capacity;
    away_available = away_capacity;
}
int main()
{
    srand(time(NULL));
    current_time = 0;
    goal_current_time = 0;
    total_person = 0;
    home_goals = 0;
    away_goals = 0;
    scanf("%d %d %d", &home_capacity, &away_capacity, &neutral_capacity);
    scanf("%d", &spectating_time);
    scanf("%d", &num_groups);
    for (int j = 0; j < num_groups; j++)
    {
        num_group_ptr[j] = (num_group *)malloc(sizeof(num_group));
        num_group_ptr[j]->id = j;
        scanf("%d", &num_group_ptr[j]->num_person);
        for (int i = 0; i < num_group_ptr[j]->num_person; i++)
        {
            spectator_ptr[total_person] = (person *)malloc(sizeof(person));
            spectator_ptr[total_person]->curr_stat = -1;
            spectator_ptr[total_person]->id = total_person;
            scanf("%s %c %d %d %d", spectator_ptr[total_person]->name, &spectator_ptr[total_person]->fan, &spectator_ptr[total_person]->reach_time, &spectator_ptr[total_person]->pat_time, &spectator_ptr[total_person]->num_goals);
            total_person++;
        }
    }
    fflush(stdout);
    scanf("%d", &goal_chances);
    for (int cal = 0; cal < goal_chances; cal++)
    {
        match_ptr[cal] = (match *)malloc(sizeof(match));
        scanf(" %c %d %lf", &match_ptr[cal]->team, &match_ptr[cal]->g_time, &match_ptr[cal]->prob);
    }
    printf("Simulation going to begin\n");
    semaphore_init();
    int st_total_count=total_person+1;
    st_count =0;
    for (int j = 0; j < total_person; j++)
    {
        pthread_mutex_init(&(spectator_ptr[j]->mutex), NULL);
        spectator_ptr[j]->thr_id = pthread_create(&(spectator_ptr[j]->thread_obj), NULL, init_spectator, (void *)(&(spectator_ptr[j]->id)));
    }
    // need a thread for goal simulation
    pthread_mutex_init(&mt.mutex, NULL);
    mt.thr_id = pthread_create(&mt.thread_obj, NULL, init_match, NULL);
    while(true)
    {
        sleep(1);
        pthread_mutex_lock(&time_ke_liye);
        current_time++;
        pthread_mutex_unlock(&time_ke_liye);
        pthread_mutex_lock(&goal_time_ke_liye);
        goal_current_time++;
        pthread_mutex_unlock(&goal_time_ke_liye);
        pthread_cond_broadcast(&for_stay);
        pthread_mutex_lock(&simulation);
        if(st_count == st_total_count)
        {
            pthread_mutex_unlock(&simulation);
            break;
        }
        pthread_mutex_unlock(&simulation);
    }
    for (int i = 0; i < total_person; i++)
    {
        pthread_join(spectator_ptr[i]->thread_obj, NULL);
    }
    pthread_join(mt.thread_obj, NULL);
}