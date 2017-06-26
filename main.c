

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "homm3_res_parser.h"
#include "homm3_lod_file.h"

static void openLod(void (^onSuccess)(FILE *resfptr));
void usage();
void list();
void extractFile(const char *filename);
void extractAll();


static void openLod(void (^onSuccess)(FILE *resfptr))
{
    FILE *resfptr = NULL;

    resfptr = fopen("H3sprite.lod", "r");
    if (resfptr != NULL) {
        onSuccess(resfptr);
        fclose(resfptr);
    } else {
        printf("Open H3sprite.lod failed\n");
    }
}

void usage()
{
    printf("-l | -f <filename> | -a\n");
}

void list()
{
    openLod(^(FILE *resfptr) {
        printFileList(resfptr);
    });
}

static struct sprite * getSpriteByFileDesc(FILE *resfptr, const FileDesc desc)
{
    struct sprite * ret = NULL;
    size_t fileSize = 0;

    ResFile file = getFile(resfptr, desc, &fileSize);
    if (file != NULL) {
        ret = getSpriteFromMemory(file);
        free(file);
    }
    return ret;
}

static int extractSprite(FILE *resfptr, const FileDesc file)
{
    struct sprite *sprite = NULL;

    sprite = getSpriteByFileDesc(resfptr, file);
    if (sprite != NULL) {
        if (mkdir(file->name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
            for (int i=0; i<sprite->totalFrames; i++) {
                char outname[256];
                snprintf(outname, sizeof(outname), "%s/frame_%02d", file->name, i);
                FILE *outimg = fopen(outname, "w");
                fwrite(sprite->frames[i]->data, 1, sprite->frames[i]->dataSize, outimg);
                fclose(outimg);
                char convert[1024];
                snprintf(convert, sizeof(convert), "convert -size %dx%d -depth 8 rgba:%s %s.png; rm %s", sprite->frames[i]->imgWidth, sprite->frames[i]->imgHeight, outname, outname, outname);
                //printf("%s\n", convert);
                system(convert);
            }
        } else {
            printf("Failed make dir: %s\n", file->name);
        }

        freeSprite(sprite);
        return 0;
    } else {
        return -1;
    }
}

static int extractSpriteDummy(FILE *resfptr, const FileDesc file)
{
    struct sprite *sprite = NULL;

    sprite = getSpriteByFileDesc(resfptr, file);
    if (sprite != NULL) {
        freeSprite(sprite);
        return 0;
    } else {
        return -1;
    }
}

void extractFile(const char *filename)
{
    openLod(^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc file) {
            if (strcasecmp(file->name, filename) == 0) {
                printf("Extract sprite: %s", file->name);
                if (extractSprite(resfptr, file) == 0) {
                    printf(" ... SUCCESS\n");
                    return SUCCESS;
                } else {
                    printf(" ... FAILED\n");
                    return FAILED;
                }
            } else {
                return NEXT;
            }
        });
    });
}

void extractAll()
{
    openLod(^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc file) {
            size_t nameLen = strlen(file->name);
            if (nameLen > 4) {
                if (strcasecmp(file->name + nameLen - 4, ".def") == 0) {
                    printf("Extract sprite: %s", file->name);
                    if (extractSprite(resfptr, file) == 0) {
                        printf(" ... SUCCESS\n");
                    } else {
                        printf(" ... FAILED\n");
                    }
                }
            }

            return NEXT;
        });
    });
}

void extractFileDummy(const char *filename)
{
    openLod(^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc file) {
            if (strcasecmp(file->name, filename) == 0) {
                printf("Create sprite: %s", file->name);
                if (extractSpriteDummy(resfptr, file) == 0) {
                    printf(" ... SUCCESS\n");
                    return SUCCESS;
                } else {
                    printf(" ... FAILED\n");
                    return FAILED;
                }
            } else {
                return NEXT;
            }
        });
    });
}

void extractAllDummy()
{
    openLod(^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc file) {
            size_t nameLen = strlen(file->name);
            if (nameLen > 4) {
                if (strcasecmp(file->name + nameLen - 4, ".def") == 0) {
                    printf("Create sprite: %s", file->name);
                    if (extractSpriteDummy(resfptr, file) == 0) {
                        printf(" ... SUCCESS\n");
                    } else {
                        printf(" ... FAILED\n");
                    }
                }
            }

            return NEXT;
        });
    });
}

int main(int argc, char * const argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }

    int c;
    while ((c = getopt(argc, argv, "lf:ad:D")) != -1) {
        switch (c) {
            case 'l':
                list();
                break;
            case 'f':
                extractFile(optarg);
                break;
            case 'a':
                extractAll();
                break;
            case 'd':
                extractFileDummy(optarg);
                break;
            case 'D':
                extractAllDummy();
                break;
            default:
                usage();
                return 0;
        }
    }

    return 0;
}
