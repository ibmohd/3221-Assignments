#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "errors.h"
#include <semaphore.h>

#define DEBUG true

///////////////////////////////////////////////////////////////
            //GLOBAL VARIABLES AND DEFINITIONS
///////////////////////////////////////////////////////////////

// struct definition for alarm 
typedef struct alarm_tag{

    struct alarm_tag *link;
    int id;
    int group_id;
    int seconds;
    time_t time;
    char message[128];
    bool changedFlag;

    pthread_t assigned_thread;
    int prev_group;
    pthread_t prev_thread;
    int message_changed;

} alarm_t;

// struct definitions for display threads
typedef struct display_thread{
    int group_id;
    pthread_t tid;
    struct display_thread *link;
} display_t;

// initialize mutex, semaphores and conditions
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t display_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t display_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t alarm_group_cond = PTHREAD_COND_INITIALIZER;
sem_t sem_alarm_list;

// max length of alarm message
#define MAX_MESSAGE_LENGTH 128

// initialize alarm list and a current alarm variable
alarm_t *alarm_list = NULL;
alarm_t *current_alarm = NULL;
display_t *display_thread_list = NULL;

///////////////////////////////////////////////////////////////
                    //HELPER FUNCTIONS
///////////////////////////////////////////////////////////////

// does not handle for same ID insert
// inserts a new alarm to the alarm list by ID
void start_alarm(alarm_t *new) {

    alarm_t **head, *next;

    head = &alarm_list;
    next = *head;

    while(next != NULL){
        if(next->id >= new->id){
            new->link = next;
            *head = new;
            break;
        }

        head = &next->link;
        next = next->link;
        
    }

    if(next == NULL){
        *head = new;
        new->link = NULL;
        new->changedFlag = false;
    }

    new->time = time(NULL) + new->seconds;

}
//returns the thread with matching group id
display_t* get_display_thread(int group_id){
    display_t* current_thread = display_thread_list;
    while(current_thread != NULL && current_thread->group_id != group_id){
        current_thread = current_thread->link;
    }
    return current_thread;
}

// change values of an existing alarm
void change_alarm(alarm_t *updated) {
    
    alarm_t *alarm = alarm_list;
    display_t* display;

    while(alarm != NULL){
        if(alarm->id == updated->id){
            alarm->message_changed = strcmp(alarm->message,updated->message);
            strcpy(alarm->message, updated->message);
            alarm->seconds = updated->seconds;
            alarm->prev_group = alarm->group_id;
            alarm->group_id = updated->group_id;
            updated->time = time(NULL) + updated->seconds;
            alarm->time = updated->time;
            alarm->changedFlag = true;
            //segmentation fault if the display hasnt already been created
            alarm->prev_thread = alarm->assigned_thread;
            display = get_display_thread(updated->group_id);
            alarm->assigned_thread = display->tid;

        }
        alarm = alarm->link;
    }
}

// funtion only used for debugging
// display all alarms currently in the alarm list
void print_alarms(){
    printf("Alarms:\n");
    alarm_t *curr = alarm_list;
    while(curr != NULL){
        printf("Alarm(%d): Group(%d) %d %s\n",
        curr->id, curr->group_id, curr->seconds, curr->message);
        curr = curr->link;
    }
}

//assigns an alarm to a display thread
// void add_to_display(alarm_t* alarm, display_t* display){

//     if(display->alarm_list == NULL){
//         display->alarm_list = alarm;
//     }else{
//         alarm_t *current_alarm = display->alarm_list;
//         while(current_alarm->link != NULL){
//             current_alarm = current_alarm->link;
//         }
//         current_alarm->link = alarm;
//     }

// }

// alternate assigns an alarm to a display thread
void add_to_display(alarm_t* alarm, display_t* display){
    alarm->assigned_thread = display->tid;
}


//removes an alarm from the alarm list
void remove_alarm(alarm_t* alarm){
    alarm_t *prev = NULL;
    alarm_t *curr = alarm_list;

    while(curr != NULL){
        if(curr->id == alarm->id){
            if(prev != NULL){
                prev->link = curr->link;
            }else{
                alarm_list = curr->link;
            }
            return;
        }
        prev = curr;
        curr = curr->link;
    }
}

bool alarm_in_alarm_list(alarm_t* alarm){
    bool flag;
    alarm_t* curr = alarm_list;
    if(curr == NULL){
        flag = false;
    }else{
        while(curr != NULL){
            if(curr->id == alarm->id){
                flag = true;
            }
            curr = curr->link;
        }
    }
    return flag;
} 


///////////////////////////////////////////////////////////////
                //THREAD START ROUTINES
///////////////////////////////////////////////////////////////

// display for alarms with particular group ids
void *alarm_display(void *arg){
    int status, sleep_interval;
    display_t *self = arg;
    alarm_t *alarm = alarm_list;

    bool first_message_change = true;
    bool first_group_change_prev = true;
    bool first_group_change_new = true;

    // if there are no alarms in the list then sleep and wait for alarms to be added
    // should never occur though as the display is only created when there are alarms
    if(alarm == NULL){
        sleep_interval=1;
    }else{

        while(alarm != NULL){

            // alarm has been assigned to this particular thread
            if(alarm->assigned_thread == self->tid){
                while(alarm_in_alarm_list(alarm) && !alarm->changedFlag){
                    printf("Alarm(%d) Printed By Alarm Display Thread %d at %ld: Group(%d) %d %s\n",
                        alarm->id, pthread_self(), time(NULL), alarm->group_id, alarm->time, alarm->message);
                    sleep(5);
                }
                // alarm has been removed from the list
                if(!alarm_in_alarm_list(alarm)){
                    printf("Display Thread %d Has Stopped Printing Message of Alarm(%d) at %ld: Group(%d) %ld %s\n",
                        pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
            }

            // alarm has been changed but the group ID is still the same (message changed)
            if(alarm->changedFlag && alarm->prev_group == self->group_id && alarm->group_id == self->group_id && alarm->message_changed != 0){
                while(alarm_in_alarm_list(alarm)){
                    if(first_message_change){
                        printf("Display Thread %d Starts to Print Changed Message Alarm(%d) at %ld: Group(%d) %ld %s\n",
                            pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
                        first_message_change = false;
                    }
                    printf("Display Thread %d Prints Changed Message Of Alarm(%d) at %ld: Group(%d) %ld %s\n",
                        pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
                    sleep(5);
                }
            }

            // alarm has been changed and the group ID has also been changed
            if(alarm->changedFlag && alarm->group_id != alarm->prev_group){
                // prev thread
                if(alarm->prev_thread == self->tid && first_group_change_prev){
                    printf("Display Thread %d Has Stopped Printing Message of Alarm(%d) at %ld: Changed Group(%d) %ld %s\n",
                        pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
                    first_group_change_prev = false;
                }   

                // new thread
                if(alarm->assigned_thread == self->tid){
                    while(alarm_in_alarm_list(alarm)){
                        if(first_group_change_new){
                            printf("Display Thread %d Has Taken Over Printing Message of Alarm(%d) at %ld: Changed Group(%d) %ld %s\n",
                                pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message); 
                            first_group_change_new = false;
                        }
                        printf("Display Thread %d Prints Changed Message Of Alarm(%d) at %ld: Group(%d) %ld %s\n",
                            pthread_self(), alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
                        sleep(5);
                    }
                }
            }

            // move onto the next alarm
            alarm = alarm->link;
        }

         //no more alarms assigned to display so terminate
        if(alarm == NULL){
            printf("No More Alarms in Group(%d): Display Thread %d exiting at %ld\n",
                self->group_id, pthread_self(), time(NULL)); 
        }

    }
    sleep(sleep_interval);

    return NULL;
    }
}

// creates display threads and assigns alarms
void *alarm_group_display_creation(void *arg){

    int status, sleep_interval;
    display_t *new_thread = (display_t*)malloc(sizeof(display_t));
    display_t* current_thread;

   

    while(true){

        // Acquire semaphore
        // sem_wait(&sem_alarm_list);
        // Lock  
        status = pthread_mutex_lock(&alarm_mutex);
        if(status != 0) err_abort(status, "Lock Mutex");

        status = pthread_cond_wait(&alarm_group_cond, &alarm_mutex);
        if(status != 0) err_abort(status, "Wait on Cond");

        if(current_alarm == NULL){
            sleep_interval = 1;
        }else{

            
            
            display_t* display = get_display_thread(current_alarm->group_id);

            //check if a display thread has been created to handle the
            //current alarms group id
            if(display != NULL && display->group_id == current_alarm->group_id){
                add_to_display(current_alarm,display);
                printf("Alarm Thread Display Alarm Thread %d Assigned to Display Alarm(%d) at %ld: Group(%d) %ld %s\n",
                    display->tid, current_alarm->id, time(NULL), current_alarm->group_id, current_alarm->time, current_alarm->message);
            }else{
                //create a new thread for that group id and assign
                new_thread->group_id = current_alarm->group_id;
                new_thread->link = NULL;
                pthread_create(&new_thread->tid, NULL, alarm_display, (void*)new_thread);

                //lock display thread mutex
                status = pthread_mutex_lock(&display_thread_mutex);
                if(status != 0) err_abort(status, "Lock Mutex");

                //thread list is empty so add to head
                if(display_thread_list == NULL){
                    display_thread_list = new_thread;
                    current_thread = display_thread_list;
                }//append to the end of the thread list
                else{
                    current_thread = display_thread_list;
                    while(current_thread->link != NULL){
                        current_thread = current_thread->link;
                    }
                    current_thread->link = new_thread;
                }

                //assign the alarm here
                add_to_display(current_alarm,current_thread);

                printf("Alarm Group Display Thread Created New Display Alarm Thread %d For Alarm(%d) at %ld: Group(%d) %ld %s\n",
                    current_thread->tid, current_alarm->id, time(NULL), current_alarm->group_id, current_alarm->time, current_alarm->message);
                
                //unlock display thread mutex
                status = pthread_mutex_unlock(&display_thread_mutex);
                if(status != 0) err_abort(status, "Unlock Mutex");
            }

        }

        // Unlock
        status = pthread_mutex_unlock(&alarm_mutex);
        if(status != 0) err_abort(status, "Unlock Mutex");
        //release the semaphore
        // sem_post(&sem_alarm_list);

        sleep(sleep_interval);

    }

    return NULL;
}

// removes expired alarms from the alarm list
void *alarm_removal(void *arg){

    int status, sleep_interval;
    alarm_t *curr;


    while(true){

        // Acquire semaphore
        sem_wait(&sem_alarm_list);
        // Lock  
        status = pthread_mutex_lock(&alarm_mutex);
        if(status != 0) err_abort(status, "Lock Mutex");

        if(alarm_list == NULL){
            sleep_interval = 1;
        }else{

            curr = alarm_list;

            while(curr != NULL){
                if(time(NULL) >= curr->time){
                    remove_alarm(curr);
                    printf("Alarm Removal Thread Has Removed Alarm(%d) at %ld: Group(%d) %ld %s\n",
                        curr->id, time(NULL), curr->group_id, curr->time, curr->message);
                }
                curr = curr->link;
            }

        }

        // Unlock
        status = pthread_mutex_unlock(&alarm_mutex);
        if(status != 0) err_abort(status, "Unlock Mutex");
        //release the semaphore
        sem_post(&sem_alarm_list);
        sleep(sleep_interval);
    }

    return NULL;
}


///////////////////////////////////////////////////////////////
                        //MAIN ROUTINE
///////////////////////////////////////////////////////////////

int main() {

    pthread_t alarm_thread;
    pthread_t removal_thread;
    int status;
    alarm_t *alarm;
    char input[256];
    int alarm_id, group_id, seconds;
    char message[MAX_MESSAGE_LENGTH+1];

    sem_init(&sem_alarm_list,0,1);

    status = pthread_create(&alarm_thread, NULL, alarm_group_display_creation, (void *)43);
    if(status != 0) err_abort(status, "Create Alarm Group Display");

    status = pthread_create(&removal_thread, NULL, alarm_removal, NULL);
    if(status != 0) err_abort(status, "Create Alarm Removal");

    while (true) {
        printf("Alarm> ");

        // Exit loop if input is bad
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        // Add error handlling here
        alarm = (alarm_t *)malloc(sizeof(alarm_t));
        if(alarm == NULL) errno_abort("Allocate Alarm");

        if (sscanf(input, "Start_Alarm(%d): Group(%d) %d %128[^\n]",
                   &alarm->id, &alarm->group_id, &alarm->seconds, alarm->message) == 4) {
            
            // Acquire semaphore
            sem_wait(&sem_alarm_list);
            // Lock  
            status = pthread_mutex_lock(&alarm_mutex);
            if(status != 0) err_abort(status, "Lock Mutex");

            start_alarm(alarm);
            printf("Alarm(%d) Inserted by Main Thread %d Into Alarm List at %ld: Group(%d) %ld %s\n",
                alarm->id, pthread_self(), time(NULL), alarm->group_id, alarm->time, alarm->message);
            
            //set the current alarm to be the alarm to be inserted
            current_alarm = alarm;
            status = pthread_cond_signal(&alarm_group_cond);
            if(status != 0) err_abort(status, "Signal Condition");
            
            // Unlock
            status = pthread_mutex_unlock(&alarm_mutex);
            if(status != 0) err_abort(status, "Unlock Mutex");
            //release the semaphore
            sem_post(&sem_alarm_list);
            
        } else if (sscanf(input, "Change_Alarm(%d): Group(%d) %d %128[^\n]",
                          &alarm->id, &alarm->group_id, &alarm->seconds, alarm->message) == 4) {
            

            // Lock  
            status = pthread_mutex_lock(&alarm_mutex);
            if(status != 0) err_abort(status, "Lock Mutex");

            change_alarm(alarm);
            printf("Alarm(%d) Changed at %ld: Group(%d) %ld %s\n",
                alarm->id, time(NULL), alarm->group_id, alarm->time, alarm->message);
            
            // Unlock
            status = pthread_mutex_unlock(&alarm_mutex);
            if(status != 0) err_abort(status, "Unlock Mutex");

    #if DEBUG
        } else if(strcmp(input,"Print")==10){
            print_alarms();
    #endif
        } else {
            printf("Invalid alarm request\n");
            free(alarm);
        }
    }

    return 0;
}
