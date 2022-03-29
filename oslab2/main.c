// TODO neki sigaction postaviti? DEFINITIVNO OBRADI SIGINT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

int seg_id;
int *shared_variable;
int global_variable;
int inputs_num;

void *thread_out() {
    printf("Pokrenuta izlazna dretva\n");
    FILE *output = fopen("output.txt", "w");
    int out;
    bool awaiting_number = true;
    for (int i = 0; i < inputs_num; i++) {

        while (!awaiting_number) {
            if (global_variable == 0) awaiting_number = true;
        }

        do {
            out = global_variable;
        } while (out == 0);
        awaiting_number = false;
        fprintf(output, "%d\n", out);
        printf("Broj %d upisan u datoteku\n", out);
        global_variable = 0;
    }
    fclose(output);
    printf("Završila izlazna dretva\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("At least one argument required!\n");
        exit(0);
    }
    printf("Pokrenut glavni proces\n");
    srand(time(NULL));

    seg_id = shmget(IPC_PRIVATE, sizeof(int), 0600);
    if (seg_id == -1) exit(1);
    shared_variable = (int *) shmat(seg_id, NULL, 0);
    global_variable = 0;

    inputs_num = (int) strtol(argv[1], &argv[2], 10); // converts string len=1 at argv[1] to int base 10
    if (fork() == 0) {  // sad smo u drugom procesu, drugoj dretvi (radnoj - ona obraduje broj)
        pthread_t thr_id;
        pthread_create(&thr_id, NULL, thread_out, NULL);

        bool awaiting_number = true;

        printf("Pokrenuta radna dretva\n");
        int to_process;
        for (int i = 0; i < inputs_num; i++) {

            while (!awaiting_number) {
                if (*shared_variable == 0) awaiting_number = true;
            }

            do {
                to_process = *shared_variable;
            } while (to_process == 0);
            awaiting_number = false;
            to_process = *shared_variable;

            printf("Radna dretva: pročitan broj %d i povećan na ", to_process++);
            printf("%d\n", to_process);
            global_variable = to_process;
        }
        printf("\nZavršila radna dretva\n");

        pthread_join(thr_id, NULL);

        exit(0);
    }

    for (int i = 0; i < inputs_num; i++) {
        int sleep_for = rand() % 5;
        for (int j = 0; j < sleep_for; j++) {
            sleep(1);
        }
        int num_generated = rand() % 100 + 1;
        printf("\nUlazna dretva: broj %d\n", num_generated);
        *shared_variable = num_generated;
        sleep(1); // wait a second to make sure working(process) thread read shared_variable
        *shared_variable = 0;

    }
    printf("Završila ulazna dretva\n");
    (void) wait(NULL);
    return 0;
}