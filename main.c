

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "homm3_res_parser.h"
#include "homm3_lod_file.h"

#ifdef LINUX
size_t strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';		/* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);	/* count does not include NUL */
}
#endif

static void openLod(const char *path, void (^onSuccess)(FILE *resfptr));
void usage();
void extractFile(const char *lodPath, const char *filename);
void extractAll(const char *lodPath, const char *filename);
void extractFileDummy(const char *lodPath, const char *filename);
void extractAllDummy(const char *lodPath, const char *filename);
void printFileList(const char *lodPath, const char *filename);


static void openLod(const char *path, void (^onSuccess)(FILE *resfptr))
{
    FILE *resfptr = NULL;

    resfptr = fopen(path, "r");
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

static struct sprite * getSpriteByFileDesc(FILE *resfptr, const FileDesc desc)
{
    struct sprite * ret = NULL;

    ResFile file = getFile(resfptr, desc);
    if (file != NULL) {
        ret = getSpriteFromMemory(file, desc->size);
        freeFile(file);
    }
    return ret;
}

static struct image * getImageByFileDesc(FILE *resfptr, const FileDesc desc)
{
    struct image * ret = NULL;

    ResFile file = getFile(resfptr, desc);
    if (file != NULL) {
        ret = getRGBImageFromMemory(file, desc->size);
        freeFile(file);
    }
    return ret;
}

static int extractSprite(FILE *resfptr, const FileDesc desc)
{
    struct sprite *sprite = NULL;

    sprite = getSpriteByFileDesc(resfptr, desc);
    if (sprite != NULL) {
        if (mkdir(desc->name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
            for (int i=0; i<sprite->totalFrames; i++) {
                char outname[256];
                snprintf(outname, sizeof(outname), "%s/frame_%02d", desc->name, i);
                FILE *outimg = fopen(outname, "w");
                fwrite(sprite->frames[i]->data, 1, sprite->frames[i]->dataSize, outimg);
                fclose(outimg);

                char convert[1024];
                snprintf(convert, sizeof(convert), "convert -size %dx%d -depth 8 rgba:%s %s.png; rm %s", sprite->frames[i]->imgWidth, sprite->frames[i]->imgHeight, outname, outname, outname);
                system(convert);
            }
        } else {
            printf("Failed make dir: %s\n", desc->name);
        }

        freeSprite(sprite);
        return 0;
    } else {
        return -1;
    }
}

static int extractSpriteDummy(FILE *resfptr, const FileDesc desc)
{
    struct sprite *sprite = NULL;

    sprite = getSpriteByFileDesc(resfptr, desc);
    if (sprite != NULL) {
        freeSprite(sprite);
        return 0;
    } else {
        return -1;
    }
}

static int extractImage(FILE *resfptr, const FileDesc desc)
{
    struct image *image = NULL;

    image = getImageByFileDesc(resfptr, desc);
    if (image != NULL) {
        char outname[256];
        snprintf(outname, sizeof(outname), "%s", desc->name);
        FILE *outimg = fopen(outname, "w");
        fwrite(image->data, 1, image->dataSize, outimg);
        fclose(outimg);
        freeImage(image);

        char convert[1024];
        snprintf(convert, sizeof(convert), "convert -size %dx%d -depth 8 rgba:%s %s.png; rm %s", image->width, image->height, outname, outname, outname);
        system(convert);

        return 0;
    } else {
        return -1;
    }
}

void extractFile(const char *lodPath, const char *filename)
{
    openLod(lodPath, ^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc desc) {
            if (strcasecmp(desc->name, filename) == 0) {
                size_t nameLen = strlen(desc->name);
                if (nameLen > 4) {
                    if (strcasecmp(desc->name + nameLen - 4, ".def") == 0) {
                        printf("Extract sprite: %s", desc->name);
                        if (extractSprite(resfptr, desc) == 0) {
                            printf(" ... SUCCESS\n");
                            return SUCCESS;
                        } else {
                            printf(" ... FAILED\n");
                            return FAILED;
                        }
                    } else if (strcasecmp(desc->name + nameLen - 4, ".pcx") == 0) {
                        printf("Extract image: %s", desc->name);
                        if (extractImage(resfptr, desc) == 0) {
                            printf(" ... SUCCESS\n");
                            return SUCCESS;
                        } else {
                            printf(" ... FAILED\n");
                            return FAILED;
                        }
                    } else {
                        printf("Unsupported file type: %s\n", desc->name + nameLen - 4);
                    }
                }
                return FAILED;
            } else {
                return NEXT;
            }
        });
    });
}

void extractAll(const char *lodPath, const char *filename)
{
    openLod(lodPath, ^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc desc) {
            size_t nameLen = strlen(desc->name);
            if (nameLen > 4) {
                if (strcasecmp(desc->name + nameLen - 4, ".def") == 0) {
                    printf("Extract sprite: %s", desc->name);
                    if (extractSprite(resfptr, desc) == 0) {
                        printf(" ... SUCCESS\n");
                    } else {
                        printf(" ... FAILED\n");
                    }
                } else if (strcasecmp(desc->name + nameLen - 4, ".pcx") == 0) {
                    printf("Extract image: %s", desc->name);
                    if (extractImage(resfptr, desc) == 0) {
                        printf(" ... SUCCESS\n");
                        return SUCCESS;
                    } else {
                        printf(" ... FAILED\n");
                        return FAILED;
                    }
                } else {
                    printf("Unsupported file type: %s\n", desc->name + nameLen - 4);
                }
            }

            return NEXT;
        });
    });
}

void extractFileDummy(const char *lodPath, const char *filename)
{
    openLod(lodPath, ^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc desc) {
            if (strcasecmp(desc->name, filename) == 0) {
                printf("Create sprite: %s", desc->name);
                if (extractSpriteDummy(resfptr, desc) == 0) {
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

void extractAllDummy(const char *lodPath, const char *filename)
{
    openLod(lodPath, ^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc desc) {
            size_t nameLen = strlen(desc->name);
            if (nameLen > 4) {
                if (strcasecmp(desc->name + nameLen - 4, ".def") == 0) {
                    printf("Create sprite: %s", desc->name);
                    if (extractSpriteDummy(resfptr, desc) == 0) {
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

void printFileList(const char *lodPath, const char *filename)
{
    openLod(lodPath, ^(FILE *resfptr) {
        forEachFile(resfptr, ^(const FileDesc desc) {
            printf("%s\n", desc->name);
            return NEXT;
        });
    });
}

int main(int argc, char * const argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }

    char lodPath[4096];
    strlcpy(lodPath, "H3sprite.lod", sizeof(lodPath));

    char filename[4096];
    bzero(filename, sizeof(filename));

    void (*process)(const char *lodPath, const char *filname) = NULL;

    int c;
    while ((c = getopt(argc, argv, "i:lf:ad:D")) != -1) {
        switch (c) {
            case 'i':
                strlcpy(lodPath, optarg, sizeof(lodPath));
                break;
            case 'l':
                process = printFileList;
                break;
            case 'f':
                strlcpy(filename, optarg, sizeof(filename));
                process = extractFile;
                break;
            case 'a':
                process = extractAll;
                break;
            /* for debug */
            case 'd':
                strlcpy(filename, optarg, sizeof(filename));
                process = extractFileDummy;
                break;
            case 'D':
                process = extractAllDummy;
                break;
            /* end for debug */
            default:
                usage();
                return 0;
        }
    }

    if (process != NULL) {
        process(lodPath, filename);
    } else {
        usage();
    }

    return 0;
}
