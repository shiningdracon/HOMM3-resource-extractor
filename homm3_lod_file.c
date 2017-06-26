
#include "homm3_lod_file.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

struct h3lodFile {
    char name[16]; // null-terminated, sometimes null-padded too, sometimes padded with, well, something after the null
    uint32_t offset; // includes the header size, no preprocessing required
    uint32_t size_original; // before compression, that is
    uint32_t type; // what's in the file - probably not used by the game directly, more on that below
    uint32_t size_compressed; // how many bytes to read, starting from offset - can be zero for stored files, use size_original in such case
};

struct h3lod {
    uint32_t magic; // always 0x00444f4c, that is, a null-terminated "LOD" string
    uint32_t type; // 200 for base archives, 500 for expansion pack archives, probably completely arbitrary numbers
    uint32_t files_count; // obvious
    uint8_t unknown[80]; // 80 bytes of hell knows what - in most cases that's all zeros, but in H3sprite.lod there's a bunch of seemingly irrelevant numbers here, any information is welcome
    struct h3lodFile file_list[10000]; // file list
};


int forEachFile(FILE *resfptr, CALLBACK_RET (^callback)(const FileDesc file))
{
    struct h3lod lodfile;
    rewind(resfptr);
    if (fread(&lodfile, 1, sizeof(lodfile), resfptr) <= 0) {
        printf("%s\n", strerror(ferror(resfptr)));
        return -1;
    }

    CALLBACK_RET ret = FAILED;
    int i;
    for (i=0; i<lodfile.files_count; i++) {
        ret = callback((FileDesc)&(lodfile.file_list[i]));
        if (ret == NEXT) {
            continue;
        } else {
            break;
        }
    }

    return ret;
}

ResFile getFile(FILE *resfptr, const FileDesc desc, size_t *retSize)
{
    size_t size_to_read, size_comp, size_orig;
    struct h3lodFile *file = (struct h3lodFile *)desc;

    size_comp = file->size_compressed;
    size_orig = file->size_original;
    if (size_comp > 0) {
        // compressed
        size_to_read = size_comp;
    } else {
        size_to_read = size_orig;
    }

    unsigned char *data_buff = (unsigned char *)malloc(size_to_read);
    if (data_buff != NULL) {
        fseek(resfptr, file->offset, SEEK_SET);
        size_t len = fread(data_buff, 1, size_to_read, resfptr);
        if (len > 0) {
            if (size_comp > 0) {
                Bytef *buff_uncompressed = (Bytef *)malloc(size_orig);
                if (buff_uncompressed != NULL) {
                    uLongf z_compressed_size = size_comp;
                    uLongf z_uncompressed_size = size_orig;
                    int ret = uncompress(buff_uncompressed, &z_uncompressed_size, data_buff, z_compressed_size);
                    if (ret == Z_OK && z_uncompressed_size == size_orig) {
                        free(data_buff);
                        *retSize = size_orig;
                        return buff_uncompressed;
                    }
                    free(buff_uncompressed);
                }
            } else {
                *retSize = size_orig;
                return data_buff;
            }
        }
        free(data_buff);
    }

    return NULL;
}

FileDesc getFileDesc(FILE *resfptr, const char *name)
{
    FileDesc retFile = NULL;
    FileDesc * pRetFile = &retFile;
    int ret = forEachFile(resfptr, ^(const FileDesc f) {
        struct h3lodFile *file = (struct h3lodFile *)f;
        if (strcasecmp(file->name, name) == 0) {
            *pRetFile = malloc(sizeof(struct h3lodFile));
            memcpy(*pRetFile, file, sizeof(struct h3lodFile));
            return SUCCESS;
        } else {
            return NEXT;
        }
    });

    if (ret == 0) {
        return retFile;
    } else {
        return NULL;
    }
}

void printFileList(FILE *resfptr)
{
    forEachFile(resfptr, ^(const FileDesc f) {
        struct h3lodFile *file = (struct h3lodFile *)f;
        printf("%s\n", file->name);
        return NEXT;
    });
}
