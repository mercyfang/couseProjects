//
//  main.c
//  dos_scandisk.c
//
//  Created by Mercy on 12/2/16.
//  Copyright © 2016 Mercy. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

// Offset of string to be printed for size inconsistency.
int offset = 0;
// This method goes through all directories and each file to do the tasks needed.
void follow_dir(uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb, int* clusterArray, char* lenInconsis) {
    struct direntry *dirent;
    int d, i;
    /* cluster_to_addr returns the memory location where the memory mapped
     cluster actually starts */
    dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
    while (1) {
        for (d = 0; d < bpb->bpbBytesPerSec * bpb->bpbSecPerClust; d += sizeof(struct direntry)) {
            char name[9];
            char extension[4];
            uint16_t file_cluster;
            name[8] = ' ';
            extension[3] = ' ';
            memcpy(name, &(dirent->deName[0]), 8);
            memcpy(extension, dirent->deExtension, 3);
            if (name[0] == SLOT_EMPTY)
                return;
            
            /* skip over deleted entries */
            if (((uint8_t)name[0]) == SLOT_DELETED)
                continue;
            
            /* names are space padded - remove the spaces */
            for (i = 8; i > 0; i--) {
                if (name[i] == ' ')
                    name[i] = '\0';
                else
                    break;
            }

            /* remove the spaces from extensions */
            for (i = 3; i > 0; i--) {
                if (extension[i] == ' ')
                    extension[i] = '\0';
                else
                    break;
            }
            
            /* don't print "." or ".." directories */
            if (strcmp(name, ".")==0) {
                dirent++;
                continue;
            }
            if (strcmp(name, "..")==0) {
                dirent++;
                continue;
            }

            if ((dirent->deAttributes & ATTR_VOLUME) != 0) {
            } else if ((dirent->deAttributes & ATTR_DIRECTORY) != 0) {
                // We are still going through directories.
                file_cluster = getushort(dirent->deStartCluster);
                clusterArray[file_cluster] = 1;
                follow_dir(file_cluster, image_buf, bpb, clusterArray, lenInconsis);
            } else {
                // Check if the length in the directory entry is inconsistent with the length in the FAT.
                // FAT: a file has # clusters of 512 bytes. Totally # * 512 bytes comparing with directory entry file byte number.

                // When we get to a file, recursively go through all the clusters this file
                //owns and mark them as visited in cluster array.
                uint16_t clusterOfFile = getushort(dirent->deStartCluster);
                int countOfClusters = 0;
                while (!is_end_of_file(clusterOfFile)) {
                    // Mark the entry in cluster array as visited.
                    countOfClusters++;
                    clusterArray[clusterOfFile] = 1;
                    clusterOfFile = get_fat_entry(clusterOfFile, image_buf, bpb);
                }
                if (is_end_of_file(cluster)) {
                    countOfClusters++;
                    clusterArray[clusterOfFile] = 1;
                }
                // Check for file size inconsistency.
                uint32_t fileSizeInFAT = countOfClusters * (bpb->bpbBytesPerSec) * (bpb->bpbSecPerClust);
                uint32_t sizeClusters = (getulong(dirent->deFileSize) + ((bpb->bpbBytesPerSec) * (bpb->bpbSecPerClust) - 1)) / (bpb->bpbBytesPerSec) * (bpb->bpbSecPerClust);
                if (sizeClusters != countOfClusters) {
                    // Append printed strings for file size inconsistency.
                    int count=0;
                    char string[50]= "";
                    sprintf(string, "%s.%s %u %u\n", name, extension, getulong(dirent->deFileSize), fileSizeInFAT);
                    strcat(lenInconsis, string);
                    count++;
                }
            }
            dirent++;
        }
        if (cluster == 0) {
            // root dir is special.
            dirent++;
        } else {
            // get_fat_entry returns the value from the FAT entry for clusternum.
            cluster = get_fat_entry(cluster, image_buf, bpb);
            dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
        }
    }
}

/* write the values into a directory entry */
void write_dirent(struct direntry *dirent, char *filename,
                  uint16_t start_cluster, uint32_t size) {
    char *p, *p2;
    char *uppername;
    int len, i;
    
    /* clean out anything old that used to be here */
    memset(dirent, 0, sizeof(struct direntry));
    
    /* extract just the filename part */
    uppername = strdup(filename);
    p2 = uppername;
    for (i = 0; i < strlen(filename); i++) {
        if (p2[i] == '/' || p2[i] == '\\') {
            uppername = p2+i+1;
        }
    }
    
    /* convert filename to upper case */
    for (i = 0; i < strlen(uppername); i++) {
        uppername[i] = toupper(uppername[i]);
    }
    
    /* set the file name and extension */
    memset(dirent->deName, ' ', 8);
    p = strchr(uppername, '.');
    memcpy(dirent->deExtension, "___", 3);
    if (p == NULL) {
        fprintf(stderr, "No filename extension given - defaulting to .___\n");
    } else {
        *p = '\0';
        p++;
        len = strlen(p);
        if (len > 3) len = 3;
        memcpy(dirent->deExtension, p, len);
    }
    if (strlen(uppername)>8) {
        uppername[8]='\0';
    }
    memcpy(dirent->deName, uppername, strlen(uppername));
    free(p2);
    
    /* set the attributes and file size */
    dirent->deAttributes = ATTR_NORMAL;
    putushort(dirent->deStartCluster, start_cluster);
    putulong(dirent->deFileSize, size);
    
    /* a real filesystem would set the time and date here, but it's
     not necessary for this coursework */
}

// This method finds a free slot in the directory, and write the directory entry.
void create_dirent(struct direntry *dirent, char *filename,
                   uint16_t start_cluster, uint32_t size,
                   uint8_t *image_buf, struct bpb33* bpb) {
    while(1) {
        if (dirent->deName[0] == SLOT_EMPTY) {
            /* we found an empty slot at the end of the directory */
            write_dirent(dirent, filename, start_cluster, size);
            dirent++;
            
            /* make sure the next dirent is set to be empty, just in
             case it wasn't before */
            memset((uint8_t*)dirent, 0, sizeof(struct direntry));
            dirent->deName[0] = SLOT_EMPTY;
            return;
        }
        if (dirent->deName[0] == SLOT_DELETED) {
            /* we found a deleted entry - we can just overwrite it */
            write_dirent(dirent, filename, start_cluster, size);
            return;
        }
        dirent++;
    }
}

// Unreferenced cluster should be added to linked list in FAT, one per lost file.
// How many lost files. How long each is. What the first block of each is.
void findLostFiles(uint8_t* image_buf, struct bpb33* bpb, int* clusterArray, int numOfClusters){
    int numLost = 0;
    for (int i=2;i<numOfClusters;i++) {
        char filename[25]= "";
        if (get_fat_entry(i, image_buf, bpb) != CLUST_FREE && (clusterArray[i] == 0)) {
            // Go through the linked list and change the entry in array to visited.
            clusterArray[i] = 1;
            uint16_t startOfCluster = i;
            int count=0;
            uint16_t clusterNumber = i;
            while (!is_end_of_file(clusterNumber)) {
                clusterNumber = get_fat_entry(clusterNumber, image_buf, bpb);
                if (clusterNumber < numOfClusters) {
                    clusterArray[clusterNumber] = 1;
                }
                count++;
            }
            if (is_end_of_file(clusterNumber)){
                if (clusterNumber < numOfClusters) {
                    clusterArray[clusterNumber] = 1;
                }
                numLost++;
            }
            printf("Lost File: %d %d\n", startOfCluster, count);
            // Create directory entry for each lost file so the files can be recovered.
            // First find the memory location of the root directory.
            struct direntry* dirent = (struct direntry*) cluster_to_addr(0, image_buf, bpb);

            sprintf(filename, "%s%i%s", "found", numLost, ".dat");
            uint32_t size = bpb->bpbSecPerClust * bpb->bpbBytesPerSec * count;
            create_dirent(dirent, filename, startOfCluster, size, image_buf, bpb);

        }
    }
}

void usage() {
    fprintf(stderr, "Usage: dos_ls <imagename>\n");
    exit(1);
}

int main(int argc, char** argv) {
    // Takes a disk image file and create a memory map from it.
    uint8_t *image_buf;
    int fd;
    struct bpb33* bpb;
    if (argc < 2 || argc > 2) {
        usage();
    }
    char lenInconsis[550] = "";
    image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);
    // Create a cluster array to store another copy of FAT.
    int numOfClusters = bpb->bpbSectors / bpb->bpbSecPerClust;
    int clusterArray[numOfClusters];
    for (int i=0;i<numOfClusters;i++) {
        clusterArray[i]=0;
    }
    // Walk directory tree and check FAT linked list for each file.
    follow_dir(0, image_buf, bpb, clusterArray, lenInconsis);
    printf("Unreference:");
    for (int i=2;i<numOfClusters;i++) {
        // If the cluster are neither free in the FAT nor used in any file.
        if (get_fat_entry(i, image_buf, bpb) != CLUST_FREE && (clusterArray[i] == 0)) {
            printf(" %d", i);
        }
    }
    printf("\n");
    findLostFiles(image_buf, bpb, clusterArray, numOfClusters);
    // Print out file size inconsistency.
    printf("%s", lenInconsis);
    close(fd);
    exit(0);
    // Modify FAT to free up the clusters that are past the end of the file.

    return 0;
}
