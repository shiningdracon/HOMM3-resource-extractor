
#ifndef __HOMM3Res__homm3_lod_file__
#define __HOMM3Res__homm3_lod_file__

#include <stdio.h>
#include <stdint.h>

typedef struct FileDesc_t {
    char name[16];
    char gap_0[4];
    uint32_t size;
} *FileDesc;

typedef uint8_t* ResFile;

typedef enum {
    FAILED = -1,
    SUCCESS = 0,
    NEXT = 1,
} CALLBACK_RET;

int forEachFile(FILE *resfptr, CALLBACK_RET (^callback)(const FileDesc file));
FileDesc getFileDesc(FILE *resfptr, const char *name);
void freeFileDesc(FileDesc desc);
ResFile getFile(FILE *resfptr, const FileDesc file);
void freeFile(ResFile file);

#endif /* defined(__HOMM3Res__homm3_lod_file__) */
