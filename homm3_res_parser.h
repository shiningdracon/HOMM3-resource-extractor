
#ifndef __HOMM3Res__homm3_res_parser__
#define __HOMM3Res__homm3_res_parser__

#include <stdio.h>
#include <stdint.h>

struct image {
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    uint8_t *data;
};

struct decodedFrame {
    uint32_t id;
    uint32_t leftMargin;
    uint32_t topMargin;
    uint32_t fullWidth;
    uint32_t fullHeight;
    uint32_t imgWidth;
    uint32_t imgHeight;
    uint32_t dataSize;
    uint8_t *data;
};

struct sequence {
    uint32_t size;
    int *frame_ids;
};

struct sprite {
    uint32_t size;
    struct sequence *frameSequences;
    uint32_t totalFrames;
    struct decodedFrame **frames;
};

#ifdef __cplusplus
extern "C" {
#endif
    
    //struct image * getRGBImage(FILE *resfptr, char const * name);
    struct image * getRGBImageFromMemory(uint8_t *mem, size_t size);
    void freeImage(struct image * image);

    struct sprite * getSpriteFromMemory(uint8_t *mem, size_t size);
    void freeSprite(struct sprite * sprite);
    
#ifdef __cplusplus
}
#endif

#endif /* defined(__HOMM3Res__homm3_res_parser__) */
