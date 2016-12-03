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
#include <machine/endian.h>
#include <string.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

int main(int argc, const char** argv) {
    // Takes a disk image file.
    uint8_t* image_buf;
    int fd;
    struct bpb33* bpb;
    image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);
    
    // get_fat_entry.   get number from cluster number in FAT
    // is_end_of_file.   FAT entry is end of file.
    
    // Walk directory tree and check FAT linked list for each file.
    // If the cluster are neither free in the FAT nor used in any file.
        // Unreferenced cluster should be added to linked list in FAT, one per lost file.
        // How many lost files. How long each is. What the first block of each is.
    
        // Create a directory entry so the files can be recovered.
    
    
    // If the length in the directory entry is inconsistent with the length in the FAT.
        // FAT: a file has # clusters of 512 bytes. Totally # * 512 bytes comparing with directory entry file byte number.
    
        // Modify FAT to free up the clusters that are past the end of the file.
    return 0;
}
