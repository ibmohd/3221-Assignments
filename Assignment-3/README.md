# 3221-Assignment-3

EECS 3221 Assingment3

1 - First copy the files "new_alarm_cond.c", and "errors.h" into your own directory.

2 - To compile the program "new_alarm_cond.c", use the following command:

    cc new_alarm_cond.c -D_POSIX_PTHREAD_SEMANTICS -lpthread
    
    OR
    
    type in the make command in the terminal making sure you are in the same directory as
    the make file before executing the command.

3 - Type "a.out" to run the executable code.

4 - At the prompt "Alarm>", type in "Start_Alarm" or "Change_Alarm" followed by the Alarm_ID in brackets, followed by a ":", followed by "Group", with the group id in brackets and then the number of seconds at which the alarm should expire, followed by the text of the message. For example:

    Alarm> Start_Alarm(1): Group(1) 20 Good Morning!
    //or
    Alarm> Change_Alarm(1): Group(2) 10 Good Night!

(To exit from the program, type Ctrl-c.)
