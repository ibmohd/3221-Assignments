# 3221-Assignment-2

EECS 3221 Assingment2

1 - First copy the files "new_alarm_mutex.c", and "errors.h" into your own directory.

2 - To compile the program "new_alarm_mutex.c", use the following command:

    cc new_alarm_mutex.c -D_POSIX_PTHREAD_SEMANTICS -lpthread

3 - Type "a.out" to run the executable code.

4 - At the prompt "ALARM>", type in "Start_Alarm" or "Change_Alarm" followed by the Alarm_ID in brackets, followed by a ":", followed by the number of seconds at which the alarm should expire, followed by the text of the message. For example:

    Alarm> Start_Alarm(1): 2 Good Morning!
    //or
    Alarm> Change_Alarm(1): 10 Good Night!

(To exit from the program, type Ctrl-d.)
