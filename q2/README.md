# The Clasico Experience
The problem ask us to simulate situation where<br>
1. Spectators are buying thier tickets
2. Teams are scoring goals
3. Spectators are leaving beacuse of some reason

**Note Bouns is implemented**
# Running the program
Either you can use: <br>
```sh
make
./q2.out
```
OR you can compile it by: <br>
```sh
gcc -pthread q2.c -o q2.out
./q2.out
```
# Working 
I have used 1 thread to simulate what happens in the match<br>
That is whether a goal is scored or not<br>
1 Thread each for a spectator<br>

Some important semaphore and mutex lock used are initialized in `semaphore_init()`
```cpp
void semaphore_init()
{
    sem_init(&away_tickets, 0, away_capacity); // number of tickets available in away box
    sem_init(&combi_home_neutral, 0, (home_capacity + neutral_capacity)); // number of tickets available in home and neutral box
    sem_init(&combi_home_away_neutral, 0, (home_capacity + neutral_capacity + away_capacity)); // number of tickets available in home, neutral and away box
    pthread_mutex_init(&lock_tickets,NULL);
    pthread_mutex_init(&time_ke_liye, NULL);
    pthread_mutex_init(&goal_time_ke_liye, NULL);
    pthread_cond_init(&for_stay,NULL);
    pthread_mutex_init(&simulation,NULL);
    home_available = home_capacity;
    neutral_available = neutral_capacity;
    away_available = away_capacity;
}
```
# Selling of ticket
Since every spectator have different choices (where to be seated) and a patience level<br>
So an Away fan will do `sem_timedwait(&away_tickets, &ts))` where ts is its patience level<br>
as soon as we find that an away ticket is available we will acquire the mutex lock and would change variable and would also deacrease the value of combi_home_away_neutral semaphore as an away ticket is sold<br>
For a neutral fan i am doing `sem_timedwait(&combi_home_away_neutral, &ts))` as soon as we find that an away ticket is available we will acquire the mutex lock and would change variable depending upon the location where we got the seat, also need to decrease the value of semaphore combi_home_neutral if ticket sold either belong to home box or neutral box else would need to decrease the value of semaphore away_tickets as the ticket sold will be of away box <br>
The process of a home fan is similar<br>

# Time
To update current time i have used mutex lock time_ke_liye and goal_time_ke_liye<br>
The current time was useful in cases where goal scoring and watching match was implemented<br> 

# Exit of specatator
1. Never got a seat<br>
    Since it never got a seat this part is trivial
2. Either spectating time is over or his team is performing poorly<br>
    For that i have used a cond variable the person wait on this<br>
    ```cpp
        pthread_mutex_lock(&goal_time_ke_liye);
        while (cur_ti > goal_current_time && home_goals < spectator_ptr[id]->num_goals)
        {
            pthread_cond_wait(&for_stay, &goal_time_ke_liye);
        }
        pthread_mutex_unlock(&goal_time_ke_liye);
    ```
    The above snippet is for the away fan it wait untill he has watched the match for the designated time for home goals has scored more goals than he would have liked<br>
    Every time a goal is scored match thread uses `pthread_cond_broadcast()` and whenever there is a change in the value of goal_current_time it also uses signals every waiting thread.
<br><br>
After exit the mutex lock `lock_tickets` is acquired to increase the desired semaphore, for example if a person at venue H leaves then the variable home_capacity will be increased, semaphore combi_home_neutral and combi_home_away_neutral will be increased. And after that lock will be increased<br>

# BONUS
For bonus part I have created a thread for each group it basically do a conditional wait, i.e it wait untill all people in the group have exited
```cpp
void *init_groups(void *ptr)
{
    int id = *((int *)ptr);
    pthread_mutex_lock(&num_group_ptr[id]->gmutex);
    while(num_group_ptr[id]->num_person > num_group_ptr[id]->people_exited)
    {
        pthread_cond_wait(&num_group_ptr[id]->gcond,&num_group_ptr[id]->gmutex);
    }
    printf(BRED"Group %d is leaving for dinner\n"ANSI_RESET,id);
    return NULL;
}
```
Everytime a spectator leaves it send the signal to its group.

# COLOR SCHEME
1. RED 
    1. When a person reaches the stadium
    2. When the group leaves for dinner
2. GREEN
    1. When a person get seat in the stadium
3. YELLOW
    1. When a person could not get a seat because its patience level was over
4. CYAN
    1. Whenever a team(A/H) scores a goal
5. BLK
    1. Whenever a team(A/H) missed a goal scoring opportunity
6. BLUE
    1. When a person exit after watching the game for spectating time.
    2. When a person exit it waits for his friends
7. MARGENTA
    1. When a person leaves beacuse his team is performing poorly