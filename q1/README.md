# An alternate course allocation Portal
In this problem we have to simulate the allocation of TA to a course, tutorial taking place and students filling their prefernces and choosing their course.<br>
<br>

## Constrain
I have used case 1 where tut is happening with 0 students attending and have added a delay of 5 seconds

## Runing the Program
Either you can use :<br>
```sh
make
./q1.out
```
Or you can use: <br>
```sh
gcc -pthread q1.c -o q1.out
./q1.out
```

# Working
I have made one thread for each student and each course, treating information about LAB and TAs as a resource.<br>
Since information about courses {number of slots left , course status} is shared among among students so need a mutex lock for each course <br>
Also information about LABS {number of TAs left and their status} is shared among courses so need a mutex lock for each lab<br>

## FOR STUDENT THREAD
The thread sleeps untill the student has filled the preference<br>.
After that it looks for a seat in a particular tut<br>
I have used 2*number of courses conditional variable declared as `ready_for_allocation` , `tut_is_over` to prevent busy waiting, that is for each course i have 2 cond variable<br>
Students have to wait untill the empty slots are greater than 0 or the course is still alive (for the course of their preference) <br>
The code for that is <br>
```cpp
    while (courses_ptr[course_id]->slot_decided <= 0 && courses_ptr[course_id]->status <= 0)
    {
        pthread_cond_wait(&courses_ptr[course_id]->ready_for_allocation, &courses_ptr[course_id]->mutex);
        // will sleep untill a signal is received
        // course send signal when they have decided the slots and when the course is about to exit
        // course_ptr is array of struct and course_id is the id of the course of current preference
    }
```
<br>
The conditional variable `tut_is_over` is used in similar manner, students are waiting untill the tut is going on, after the tut is over course send the signal for all these waiting thread to wake up<br>
After that students take decision whether they want to opt this course or they want to withdraw.<br>
This process goes on for student untill he accept the seat or there are no more choices available for it<br>

## FOR course
Each course (in parallel) go to lab one by one and check whether they have a TA available, for this part mutex lock is used<br>
If it does find any TA it tell all the waiting courses with the help of `pthread_cond_broadcast()` and exit<br>
Else it decides the slot for a tutorial and students start coming in, 2sec are given for student to come in and then the tut begins. The tut last for 3 sec and then it sends signal to the students that were attending the tut using `pthread_cond_broadcast()` and wait for 2 more seconds and then again start looking for TAs.

# COLOR SCHEME
1. RED 
    1. When a student has filled his preference
    2. When a student exit simulation either by selecting a course or by when he doesn't have any choice
2. GREEN - 
    1. WHEN THE STUDENT HAS CHANGED ITS PRIORITY / PREFERENCE 
    2. WHEN A STUDENT WITHDRAWS FROM COURSE
3. YELLOW 
    1. WHEN A SEAT IS ALLOCATED TO A STUDENT
4. BLUE
    1. NUMBER OF SLOTS DECIDED FOR A TUT (this was a random number)  
    2. When a course exit because of no TAs left.
5. MARGENTA 
    1. ALLOCATION OF TA
6. CYAN 
    1. STARTING OF TUT 
    2. ENDING OF TUT
7. BLK 
    1. When a lab doesn't have any TAs left it exit the simulation