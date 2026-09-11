#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>

#define FICHIER_TEST "gros_fichier.jpg"
#define DOSSIER_TEST "mnt_demo"
#define IMG_FS "fs.img"

void cas1() {
    printf("=== CAS 1 : Création d'un gros fichier reconnu comme JPG ===\n");
    
    // 1. Création du fichier avec en-tête JPG
    int fd = open(FICHIER_TEST, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Erreur création fichier");
        return;
    }
    
    unsigned char jpg_header[] = {0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 'J', 'F', 'I', 'F', 0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00};
    write(fd, jpg_header, sizeof(jpg_header));
    
    // Ajout de 5Mo de données aléatoires
    int urandom = open("/dev/urandom", O_RDONLY);
    char buffer[4096];
    for(int i = 0; i < (5 * 1024 * 1024) / 4096; i++) {
        read(urandom, buffer, 4096);
        write(fd, buffer, 4096);
    }
    close(urandom);
    close(fd);

    // 2. Récupération des statistiques natives via stat()
    struct stat file_stat;
    if (stat(FICHIER_TEST, &file_stat) == -1) {
        perror("Erreur stat");
        return;
    }

    // st_size : taille logique (données pures)
    // st_blksize : taille d'un bloc du FS (souvent 4096)
    // st_blocks : nombre de blocs alloués (en unités de 512 octets)
    long long taille_logique = file_stat.st_size;
    long long taille_bloc_fs = file_stat.st_blksize;
    long long espace_reel_disque = file_stat.st_blocks * 512; 
    
    long long blocs_theoriques = (taille_logique + taille_bloc_fs - 1) / taille_bloc_fs;
    long long espace_theorique = blocs_theoriques * taille_bloc_fs;

    printf("- Espace pris par les données (Taille logique) : %lld octets\n", taille_logique);
    printf("- Nombre de blocs système théoriquement nécessaires : %lld blocs\n", blocs_theoriques);
    printf("- Espace théorique calculé : %lld octets\n", espace_theorique);
    printf("- Espace réel occupé sur le disque : %lld octets\n", espace_reel_disque);
}

void cas2() {
    printf("=== CAS 2 : Saturation des inodes ===\n");
    
    system("dd if=/dev/zero of=" IMG_FS " bs=1M count=5 status=none");
    system("mkfs.ext4 -F -N 1024 -I 256 " IMG_FS " > /dev/null 2>&1");
    system("mkdir -p " DOSSIER_TEST);
    system("sudo mount -o loop " IMG_FS " " DOSSIER_TEST);
    system("sudo chown $USER " DOSSIER_TEST);

    printf("Création de fichiers en boucle jusqu'à l'erreur ENOSPC...\n");
    
    int i = 1;
    char filepath[256];
    while (1) {
        snprintf(filepath, sizeof(filepath), "%s/fichier_%d", DOSSIER_TEST, i);
        int fd = open(filepath, O_WRONLY | O_CREAT, 0666);
        if (fd < 0) {
            if (errno == ENOSPC) {
                printf("Saturation atteinte ! %d fichiers créés.\n\n", i-1);
                break;
            }
        } else {
            close(fd);
            i++;
        }
    }

    // 1. Statistiques du système de fichiers (pour les inodes)
    struct statvfs fs_stat;
    if (statvfs(DOSSIER_TEST, &fs_stat) == 0) {
        long long total_inodes = fs_stat.f_files;
        long long free_inodes = fs_stat.f_ffree;
        long long inodes_utilises = total_inodes - free_inodes;
        long long espace_theorique_inodes = inodes_utilises * 256;

        printf("- Espace théorique calculé (Poids des inodes) : %lld octets\n", espace_theorique_inodes);
    }

    // 2. Statistiques du dossier lui-même (l'espace réel et sur disque)
    struct stat dir_stat;
    if (stat(DOSSIER_TEST, &dir_stat) == 0) {
        long long espace_reel_dossier = dir_stat.st_size; // Taille logique du dossier
        long long espace_reel_disque = dir_stat.st_blocks * 512; // Blocs physiques alloués
        
        printf("- Espace réel du dossier (Taille logique de l'index) : %lld octets\n", espace_reel_dossier);
        printf("- Espace réel occupé sur le disque dur (Blocs alloués) : %lld octets\n", espace_reel_disque);
    }
}

void reset() {
    printf("=== NETTOYAGE ===\n");
    remove(FICHIER_TEST);
    system("sudo umount " DOSSIER_TEST " 2>/dev/null");
    system("rm -rf " DOSSIER_TEST " " IMG_FS);
    printf("Environnement réinitialisé.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s {1|2|reset}\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "1") == 0) cas1();
    else if (strcmp(argv[1], "2") == 0) cas2();
    else if (strcmp(argv[1], "reset") == 0) reset();
    else printf("Option invalide.\n");

    return 0;
}