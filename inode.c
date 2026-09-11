#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Usage: %s {1|2|reset}\n", argv[0]);
        return 1;
    }
    
    struct stat st; struct statvfs vfs; char buf[64];

    if(strcmp(argv[1], "1") == 0) {
        system("printf '\\xff\\xd8\\xff\\xe0JFIF' > f.jpg; dd if=/dev/urandom bs=1M count=5 >> f.jpg 2>/dev/null");
        stat("f.jpg", &st);
        long long log = st.st_size, blk = st.st_blksize, real = st.st_blocks * 512;
        printf("- Taille logique : %lld octets\n- Blocs theoriques : %lld\n- Espace theorique : %lld octets\n- Espace reel : %lld octets\n",
               log, (log+blk-1)/blk, ((log+blk-1)/blk)*blk, real);
    }
    else if(strcmp(argv[1], "2") == 0) {
        system("dd if=/dev/zero of=fs.img bs=1M count=20 2>/dev/null; mkfs.ext4 -F -N 1024 -I 256 fs.img >/dev/null 2>&1; mkdir -p mnt");
        if(system("sudo mount -o loop fs.img mnt") != 0) return 1;
        
        system("sudo chown $USER mnt");
        
        for(int i=1; ; i++) {
            sprintf(buf, "mnt/%d", i);
            int fd = open(buf, O_CREAT, 0666);
            if(fd < 0) {
                if(errno == ENOSPC) printf("Saturation atteinte : %d fichiers\n", i-1);
                else perror("Erreur inattendue");
                break;
            }
            close(fd);
        }
        
        statvfs("mnt", &vfs); 
        stat("mnt", &st);
        printf("- Espace theorique calcule (inodes) : %llu octets\n"
               "- Espace reel du dossier (hors FS) : %lld octets\n"
               "- Espace reel sur le disque dur (blocs) : %lld octets\n",
               (unsigned long long)(vfs.f_files - vfs.f_ffree) * 256, 
               (long long)st.st_size, 
               (long long)st.st_blocks * 512);
    }
    else if(strcmp(argv[1], "reset") == 0) {
        system("sudo umount mnt 2>/dev/null; rm -rf f.jpg mnt fs.img");
        printf("Environnement nettoye.\n");
    }
    else {
        printf("Option invalide.\nUsage: %s {1|2|reset}\n", argv[0]);
        printf("  1     : Demo 1 (Gros fichier et blocs)\n");
        printf("  2     : Demo 2 (Saturation des inodes)\n");
        printf("  reset : Nettoyer l'environnement\n");
    }
    return 0;
}