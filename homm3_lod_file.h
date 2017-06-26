
#ifndef __HOMM3Res__homm3_lod_file__
#define __HOMM3Res__homm3_lod_file__

#include <stdio.h>


typedef enum {
    FAILED = -1,
    SUCCESS = 0,
    NEXT = 1,
} CALLBACK_RET;

typedef struct FileDesc_t {
    char name[16];
} *FileDesc;

typedef uint8_t* ResFile;

int forEachFile(FILE *resfptr, CALLBACK_RET (^callback)(const FileDesc file));
FileDesc getFileDesc(FILE *resfptr, const char *name);
ResFile getFile(FILE *resfptr, const FileDesc file, size_t *retSize);
void printFileList(FILE *resfptr);

#endif /* defined(__HOMM3Res__homm3_lod_file__) */
