/*
* new_alarm_mutex.c


* This is an enhancement to the alarm_mutex.c program
*/


#include <pthread.h>
#include <time.h>
#include "errors.h"




// The alarm structure now includes an
// id field which is used to order the
// alarms


typedef struct alarm_tag
{
   struct alarm_tag *link;
   int id;
   int seconds;
   time_t time; /* seconds from EPOCH */
   char message[128];
   int changedFlag;


   // **DELETE THIS BLOCK EVENTUALLY**
   // insert a field to check when
   // an alarm has been changed?
   // int Changed;
   // **DELETE THIS BLOCK EVENTUALLY**
} alarm_t;


//Mutex for alarm_thread
pthread_mutex_t alarm_mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_cond_t display_cond = PTHREAD_COND_INITIALIZER;


//Main alarm list
alarm_t *alarm_list = NULL;
//Current alarm for use in display thread
alarm_t *current_alarm = NULL;


// Takes as parameters the alarm_list
// and the alarm to be added to the list,
// where alarms are ordered by their id
void Start_Alarm(alarm_t **last, alarm_t *new){


   // retain value of last alarm
   alarm_t *prev = *last;


   // cycles through list till alarm is inserted
   // into the right position depending on its id
   while(*last != NULL){
      
       // if new alarm id is smaller then old alarm id
       // then add new alarm ahead of old alarm
       if(prev->id >= new->id){
          new->link = prev;
          *last = new;
          new->changedFlag = 0;
       }
       // last will be new alarm address
       // and new will be the next in new
       // alarms list?
       last = &new->link;
       new = new->link;
   }


   // sets the new alarm into the last
   // position in the alarm_list
   if(*last == NULL){
       *last = new;
   }


   // set expiration time
   new->time = time(NULL) + new->seconds;
}


// Takes as parameters the alarm_list
// and the alarm to be changed,
// and changes the corresponding alarm in the list
void Change_Alarm(alarm_t **prev, alarm_t *updated){


   alarm_t *alarm = *prev;


   while(alarm != NULL){


       // checks if this is the alarm
       // to be updated and updates
       if(alarm->id == updated->id){
           strcpy(alarm->message, updated->message);
           alarm->seconds = updated->seconds;
           alarm->time = time(NULL) + updated->seconds;
           updated->changedFlag = 1;
           
       }


       //move to next alarm
       alarm = alarm->link;
   }


}


void *alarm_thread(void *arg){


   alarm_t *alarm;
   int sleep_interval;
   time_t current_time;
   int status;
 


   // continually process commands until
   // exit, after which thread terminates


   while(1){


        // lock
        status = pthread_mutex_lock(&alarm_mutex);
        if(status != 0) err_abort(status, "Error: Mutex is unlocked");
        
        
        //set alarm to alarm_list
        alarm = alarm_list;


        // if list is empty then sleep for 1 second
        // so that system can accept a new alarm
        if(alarm == NULL){
            sleep_interval = 1;
        }else{

        // move alarm_list head and
        // reassign current alarm
        alarm_list = alarm->link;
        current_alarm = alarm;

        if(alarm->changedFlag == 1){

            // when alarm has been changed

            printf("Alarm Thread(%d) Has Informed Display Thread(%d) That It Should Start Printing Changed Message at %ld: %d %s\n ",
                pthread_self(),
                // ALTER 
                // This should be the ID of display thread
                pthread_self(),
                // ALTER
                time(NULL),
                alarm->time,
                alarm->message);

        }else{

            // newly inserted alarm

            // FIND A WAY TO ASSIGN DISPLAY THREADS HERE 
            printf("New Display Thread %d Created at %ld\n",
                // ALTER 
                // This should be the ID of display thread
                pthread_self(),
                // ALTER
                time(NULL));
            
            printf("Alarm(%d) Assigned to Display Thread(%d) at %ld: %d %s\n",
                alarm->id,
                pthread_self(),
                time(NULL),
                alarm->time,
                alarm->message
                )

        }


        // Signal the display thread
        status = pthread_cond_signal(&display_cond);
        if (status != 0) err_abort(status, "Error: No Condition Signal");


        }


        // unlock
        status = pthread_mutex_unlock(&alarm_mutex);
        if(status != 0) err_abort(status, "Error: Mutex is locked");
        sleep(sleep_interval);
   }


}


void *display_thread(void *arg){


   alarm_t *alarm;
   int status;
   time_t current_time;
   // Counts the number of alarms associated with thread?
   int alarmCount;


   while(1){


      // lock
      status = pthread_mutex_lock(&alarm_mutex);
      if(status != 0) err_abort(status, "Error: Mutex is unlocked");




      // waiting for alarm_thread to signal when an alarm is ready
      status = pthread_cond_wait(&display_cond, &alarm_mutex);
      if(status != 0) err_abort(status, "Error: Waiting on condition");

      // Sets the alarm display will work with to the current alarm
      alarm = current_alarm;


      // print every 5 seconds while alarm isnt expired
      while(alarm->time > time(NULL)){

           int firstDisplayFlag = 0;

           if(alarm->changedFlag == 1){

               // Prints message when alarm is first changed 
               if(firstDisplayFlag == 0){
                printf("Display Thread(%d) Has Started to Print Changed Message at %ld: %ld %s\n",
                pthread_self(),
                time(NULL),
                alarm->time,
                alarm->message);
                // change flag value for next iteration
                firstDisplayFlag = 1;    
               } 

               printf("");
               sleep(5);


           }else{

               printf("Display Thread Printing Message Of Alarm(%d): %s\n",
               alarm->id,
               alarm->message);
               sleep(5);

           }
      
      }

      // Alarm has expired
      printf("Alarm(%d) Expired; Display Thread(%d) Stopped Printing Alarm Message at %ld: %ld %s\n",
        alarm->id,
        pthread_self(),
        time(NULL),
        alarm->time,
        alarm->message,
      );

      // No more alarms so terminate thread?
      if(alarm == NULL){
        printf("Display Thread Terminated(%d) at %ld", pthread_self(), time(NULL));
      }  
      
      // unlock
      status = pthread_mutex_unlock(&alarm_mutex);
      if(status != 0) err_abort(status, "Error: Mutex is locked");


    //   free(alarm);
   }
}


int main(int argc, char *argv[]){


   int status;
   char line[128];
   alarm_t *alarm;
   pthread_t thread;
   pthread_t display;


   // initialize threads
   // alarm thread
   status = pthread_create(&thread, NULL, alarm_thread, NULL);
   if(status != 0) err_abort(status, "Create Alarm Thread");
   // display thread
   status = pthread_create(&display, NULL, display_thread, NULL);
   if(status != 0) err_abort(status, "Create Display Thread");


   while(1){


       printf("Alarm> ");
       if (fgets(line, sizeof(line), stdin) == NULL)
           exit(0);
       if (strlen(line) <= 1)
           continue;
       alarm = (alarm_t *)malloc(sizeof(alarm_t));
       if (alarm == NULL)
           errno_abort("Allocate alarm");


       /*
       * Parse input line into seconds (%d) and a message
       * (%128[^\n]), consisting of up to 128 characters
       * separated from the seconds by whitespace.
       */
       if ((sscanf(line, "Start_Alarm(%d): %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3) && (sscanf(line, "Change_Alarm(%d): %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
       {
           fprintf(stderr, "Bad command\n");
           free(alarm);
           continue;
       }
       else if (!(sscanf(line, "Start_Alarm(%d): %d %128[^\n]", &alarm->id, &alarm->seconds, alarm->message) < 3))
       {
           //locks
           status = pthread_mutex_lock(&alarm_mutex);
           if (status != 0)
               err_abort(status, "Error: Mutex is unlocked");


           //Add alarm using Start_Alarm
           Start_Alarm(&alarm_list, alarm);
           printf("Alarm(%d) Inserted by Main Thread(%d) Into Alarm list at %ld: %d %s\n",
                alarm->id, pthread_self(), time(NULL), alarm->time, alarm->message);


           //unlocks
           status = pthread_mutex_unlock(&alarm_mutex);
           if (status != 0)
               err_abort(status, "Error: Mutex is locked");
       }
       else
       {
           //locks
           status = pthread_mutex_lock(&alarm_mutex);
           if (status != 0)
               err_abort(status, "Error: Mutex is unlocked");


           //Change alarm using Change_Alarm
           Change_Alarm(&alarm_list, alarm);
           printf("Alarm(%d) Changed at <%ld>: %d %s\n", alarm->id, time(NULL), alarm->time, alarm->message);

           //unlocks
           status = pthread_mutex_unlock(&alarm_mutex);
           if (status != 0)
               err_abort(status, "Error: Mutex is locked");
       }


        if(alarm->time < time(NULL)){
            // alarm has expired so print message and remove it
            printf("Alarm(%d): Alarm Expired at %ld: Alarm removed from alarm list\n",
            alarm->id,
            time(NULL));

            free(alarm)
        }



   }
}
