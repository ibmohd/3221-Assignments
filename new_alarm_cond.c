#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MESSAGE_LENGTH 128

void start_alarm(int alarm_id, int group_id, int time, char* message) {
    printf("Starting alarm: Alarm_ID=%d Group_ID=%d Time=%d Message=%s\n",
           alarm_id, group_id, time, message);
    // TODO: Add implementation to start alarm
}

void change_alarm(int alarm_id, int group_id, int time, char* message) {
    printf("Changing alarm: Alarm_ID=%d Group_ID=%d Time=%d Message=%s\n",
           alarm_id, group_id, time, message);
    // TODO: Add implementation to change alarm
}

int main() {
    char input[256];
    int alarm_id, group_id, time;
    char message[MAX_MESSAGE_LENGTH+1];

    while (1) {
        printf("Alarm> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        if (sscanf(input, "Start_Alarm(%d): Group(%d) %d %128[^\n]",
                   &alarm_id, &group_id, &time, message) == 4) {
            start_alarm(alarm_id, group_id, time, message);
        } else if (sscanf(input, "Change_Alarm(%d): Group(%d) %d %128[^\n]",
                          &alarm_id, &group_id, &time, message) == 4) {
            change_alarm(alarm_id, group_id, time, message);
        } else {
            printf("Invalid alarm request\n");
        }
    }

    return 0;
}
