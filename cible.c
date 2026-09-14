#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234
#define SHM_SIZE 128

int main() {
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) return 1;

    char *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (char *) -1) return 1;

    strncpy(shm_ptr, "Donnees initiales du processus cible.", SHM_SIZE - 1);
    shm_ptr[SHM_SIZE - 1] = '\0'; 

    while (1) {
        printf("Contenu actuel : %s\n", shm_ptr);
        sleep(1);
    }
    return 0;
}