
#include "homm3_res_parser.h"
#include "homm3_lod_file.h"
#include <stdlib.h>
#include <string.h>

#define FMT_RGBA

//PCX
struct h3pcxHeader {
    uint32_t bitmap_size; // indexed: equals to width*height - 1 byte per pixel. bgr: equals to 3*width*height - 3 bytes per pixel
    uint32_t width;
    uint32_t height;
};

struct h3pcxColor_bgr {
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

struct h3Color_indexed {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

#ifdef FMT_RGB
typedef struct h3Color_indexed PALETTE;
#endif
#ifdef FMT_RGBA
struct h3Color_rgba_indexed {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};
typedef struct h3Color_rgba_indexed PALETTE;
#endif

void freeImage(struct image *image)
{
    if (image->data != NULL) {
        free(image->data);
    }
    free(image);
}

struct image * getRGBImageFromMemory(uint8_t *mem, size_t size)
{
    struct image *retImage = NULL;

    uint8_t *pcx_file = mem;
    if (pcx_file != NULL) {
        if (size > sizeof(struct h3pcxHeader)) {
            struct h3pcxHeader *h3pcx = (struct h3pcxHeader *)pcx_file;
            retImage = (struct image *)malloc(sizeof(struct image));
            if (retImage != NULL) {
                retImage->width = h3pcx->width;
                retImage->height = h3pcx->height;
                retImage->dataSize = sizeof(PALETTE) * h3pcx->width * h3pcx->height;
                retImage->data = (uint8_t *)malloc(retImage->dataSize);
                if (retImage->data != NULL) {
                    if (size > sizeof(struct h3pcxHeader) + h3pcx->bitmap_size) {
                        if (h3pcx->bitmap_size == h3pcx->width * h3pcx->height) {
                            PALETTE color_palette[256];
                            struct h3Color_indexed *pcx_palette = (struct h3Color_indexed *)(pcx_file + sizeof(struct h3pcxHeader) + h3pcx->bitmap_size);
                            for (int i=0; i<256; i++) {
#ifdef FMT_RGBA
                                color_palette[i].r = pcx_palette[i].r;
                                color_palette[i].g = pcx_palette[i].g;
                                color_palette[i].b = pcx_palette[i].b;
                                color_palette[i].a = 255;
#endif
#ifdef FMT_RGB
                                color_palette[i] = pcx_palette[i];
#endif
                            }

                            for (uint32_t i=0; i<h3pcx->width * h3pcx->height; i++) {
                                ((PALETTE*)(retImage->data))[i] = color_palette[((uint8_t *)(pcx_file + sizeof(struct h3pcxHeader)))[i]];
                            }
                            return retImage;
                        } else if (h3pcx->bitmap_size == h3pcx->width * h3pcx->height * 3) {
                            for (uint32_t i=0; i<h3pcx->width * h3pcx->height; i++) {
                                ((PALETTE*)(retImage->data))[i].r = ((struct h3pcxColor_bgr *)(pcx_file + sizeof(struct h3pcxHeader)))[i].r;
                                ((PALETTE*)(retImage->data))[i].g = ((struct h3pcxColor_bgr *)(pcx_file + sizeof(struct h3pcxHeader)))[i].g;
                                ((PALETTE*)(retImage->data))[i].b = ((struct h3pcxColor_bgr *)(pcx_file + sizeof(struct h3pcxHeader)))[i].b;
#ifdef FMT_RGBA
                                ((PALETTE*)(retImage->data))[i].a = 255;
#endif
                            }
                            return retImage;
                        }
                    }
                    free(retImage->data);
                }
                free(retImage);
            }
        }
        free(pcx_file);
    }

    return NULL;
}

//DEF
struct h3defHeader {
    uint32_t type;
    uint32_t width;
    uint32_t height;
    uint32_t sequencesCount;
    struct h3Color_indexed palette[256];
};

struct h3defSequence {
    uint32_t type;
    uint32_t length;
    uint32_t unknown1;
    uint32_t unknown2;
    //char *names[length];
    //uint32_t offsets[length];
};

struct h3defFrameHeader {
    uint32_t data_size;
    uint32_t type;
    uint32_t width;
    uint32_t height;
    uint32_t imgWidth;
    uint32_t imgHeight;
    uint32_t x;
    uint32_t y;
};

static void freeDecodedFrame(struct decodedFrame *frame)
{
    free(frame->data);
    free(frame);
}

static int readNormalNr (int pos, int bytCon, unsigned char * str)
{
    //bool cyclic= false;
    int ret=0;
    int amp=1;
    int i=0;
    if (str)
    {
        for (i=0; i<bytCon; i++)
        {
            ret+=str[pos+i]*amp;
            amp*=256;
        }
    }

    return ret;
}

static struct decodedFrame * decodeFrame(uint8_t * FDef, PALETTE *color_palette)
{
    struct decodedFrame * ret_frame=NULL;
    struct h3defFrameHeader *frameheader = (struct h3defFrameHeader *)FDef;
    
    uint32_t BaseOffset,
    SpriteWidth, SpriteHeight, //format sprite
    i, FullHeight,FullWidth,
    TotalRowLength; // length of read segment
    int LeftMargin, RightMargin, TopMargin, BottomMargin;
    uint32_t defType2;
    
    uint8_t SegmentType;

    i=0;
    defType2 = frameheader->type;
    FullWidth = frameheader->width;
    FullHeight = frameheader->height;
    SpriteWidth = frameheader->imgWidth;
    SpriteHeight = frameheader->imgHeight;
    LeftMargin = frameheader->x;
    TopMargin = frameheader->y;
    RightMargin = FullWidth - SpriteWidth - LeftMargin;
    BottomMargin = FullHeight - SpriteHeight - TopMargin;
    i += sizeof(struct h3defFrameHeader);

    int64_t tmpSpriteWidth = SpriteWidth;
    if (LeftMargin < 0) {
        tmpSpriteWidth += LeftMargin;
    }
    if (RightMargin < 0) {
        tmpSpriteWidth += RightMargin;
    }
    if (tmpSpriteWidth > 0) {
        SpriteWidth = (uint32_t)tmpSpriteWidth;
    } else {
        // SGTWMTA.def and SGTWMTB.def
        // Don't know what these two files are, and why they have such a strage value.
        // Simply ignore them.
        return NULL;
    }

    int ftcp=0;
    
    ret_frame = (struct decodedFrame *)malloc(sizeof(struct decodedFrame));
    if (ret_frame != NULL) {
        ret_frame->leftMargin = frameheader->x;
        ret_frame->topMargin = frameheader->y;
        ret_frame->fullWidth = frameheader->width;
        ret_frame->fullHeight = frameheader->height;
        ret_frame->imgWidth = frameheader->imgWidth;
        ret_frame->imgHeight = frameheader->imgHeight;
        ret_frame->dataSize = sizeof(PALETTE) * SpriteWidth * SpriteHeight;
        ret_frame->data = (uint8_t *)malloc(ret_frame->dataSize);
        if (ret_frame->data != NULL) {
            int BaseOffsetor = BaseOffset = i;
            if (defType2 == 0) {
                for (int i=0; i<SpriteHeight; i++) {
                    for (int j=0; j<SpriteWidth; j++) {
                        ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[FDef[BaseOffset++]];
                    }
                }
                return ret_frame;
            }
            else if (defType2 == 1) {
                int *RLEntries = malloc(sizeof(int) * SpriteHeight);
                if (RLEntries != NULL) {
                    for (int i=0;i<SpriteHeight;i++) {
                        RLEntries[i]=readNormalNr(BaseOffset,4,FDef);BaseOffset+=4;
                    }
                    for (int i=0;i<SpriteHeight;i++)
                    {
                        BaseOffset=BaseOffsetor+RLEntries[i];
                        TotalRowLength=0;
                        do
                        {
                            uint32_t SegmentLength;

                            SegmentType=FDef[BaseOffset++];
                            SegmentLength=FDef[BaseOffset++];
                            if (SegmentType==0xFF)
                            {
                                for (int k=0;k<=SegmentLength;k++)
                                {
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[FDef[BaseOffset+k]];
                                    if ((TotalRowLength+k+1)>=SpriteWidth)
                                        break;
                                }
                                BaseOffset+=SegmentLength+1;////
                                TotalRowLength+=SegmentLength+1;
                            }
                            else
                            {
                                for (int k=0;k<SegmentLength+1;k++)
                                {
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[SegmentType];//
                                    //((char*)(ret->data))+='\0';
                                }
                                TotalRowLength+=SegmentLength+1;
                            }
                        }while(TotalRowLength<SpriteWidth);
                    }
                    free(RLEntries);
                    return ret_frame;
                }
            }
            else if (defType2 == 2) {
                unsigned int *RWEntries = malloc(sizeof(unsigned int) * SpriteHeight);
                if (RWEntries != NULL) {
                    for (int i=0;i<SpriteHeight;i++) {
                        BaseOffset=BaseOffsetor+i*2*(SpriteWidth/32);
                        RWEntries[i] = readNormalNr(BaseOffset,2,FDef);
                    }
                    BaseOffset = BaseOffsetor+RWEntries[0];
                    for (int i=0;i<SpriteHeight;i++) {
                        //BaseOffset = BaseOffsetor+RWEntries[i];
                        TotalRowLength=0;
                        do
                        {
                            SegmentType=FDef[BaseOffset++];
                            unsigned char code = SegmentType / 32;
                            unsigned char value = (SegmentType & 31) + 1;
                            if(code==7)
                            {
                                for(int h=0; h<value; ++h)
                                {
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[FDef[BaseOffset++]];
                                }
                            }
                            else
                            {
                                for(int h=0; h<value; ++h)
                                {
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[code];
                                }
                            }
                            TotalRowLength+=value;
                        } while(TotalRowLength<SpriteWidth);
                    }
                    free(RWEntries);
                    return ret_frame;
                }
            }
            else if (defType2==3) {
                unsigned int *RWEntries = malloc(sizeof(unsigned int) * SpriteHeight);
                if (RWEntries != NULL) {
                    for (int i=0;i<SpriteHeight;i++)
                    {
                        BaseOffset=BaseOffsetor+i*2*(SpriteWidth/32);
                        RWEntries[i] = readNormalNr(BaseOffset,2,FDef);
                    }
                    for (int i=0;i<SpriteHeight;i++)
                    {
                        BaseOffset = BaseOffsetor+RWEntries[i];
                        TotalRowLength=0;
                        do
                        {
                            SegmentType=FDef[BaseOffset++];
                            unsigned char code = SegmentType / 32;
                            unsigned char value = (SegmentType & 31) + 1;
                            if(code==7)
                            {
                                for(int h=0; h<value; ++h)
                                {
                                    if(h<-LeftMargin)
                                        continue;
                                    if(h+TotalRowLength>=SpriteWidth)
                                        break;
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[FDef[BaseOffset++]];
                                }
                            }
                            else
                            {
                                for(int h=0; h<value; ++h)
                                {
                                    if(h<-LeftMargin)
                                        continue;
                                    if(h+TotalRowLength>=SpriteWidth)
                                        break;
                                    ((PALETTE*)(ret_frame->data))[ftcp++]=color_palette[code];
                                }
                            }
                            TotalRowLength+=( LeftMargin>=0 ? value : value+LeftMargin );
                        }while(TotalRowLength<SpriteWidth);
                    }
                    free(RWEntries);
                    return ret_frame;
                }
            }
            free(ret_frame->data);
        }
        free(ret_frame);
    }
    
    return NULL;
}

void freeSprite(struct sprite * sprite)
{
    unsigned int i = 0;
    if (sprite->frameSequences != NULL) {
        for (i=0; i<sprite->size; i++) {
            if (sprite->frameSequences[i].frame_ids != NULL)
                free(sprite->frameSequences[i].frame_ids);
        }
        free(sprite->frameSequences);
    }
    if (sprite->frames != NULL) {
        for (i=0; i<sprite->totalFrames; i++) {
            if (sprite->frames[i] != NULL)
                freeDecodedFrame(sprite->frames[i]);
        }
        free(sprite->frames);
    }
    
    free(sprite);
}


struct sprite * getSpriteFromMemory(uint8_t *mem, size_t size)
{
    struct sprite *ret = (struct sprite *)malloc(sizeof(struct sprite));
    if (ret == NULL) {
        return NULL;
    }
    memset(ret, 0, sizeof(struct sprite));
    int i, totalInBlock, frame_max_number;
    PALETTE color_palette[256];
    uint32_t frameindex = 0;
    uint32_t *frameoffsets = NULL;
    struct h3defHeader *h3def = NULL;

    h3def = (struct h3defHeader *)mem;
    
    for (int i=0; i<256; i++) {
#ifdef FMT_RGBA
        color_palette[i].r = h3def->palette[i].r;
        color_palette[i].g = h3def->palette[i].g;
        color_palette[i].b = h3def->palette[i].b;
        color_palette[i].a = 255;
#endif
#ifdef FMT_RGB
        color_palette[i] = h3def->palette[i];
#endif
    }
    
#ifdef FMT_RGBA
#define SET_COLOR(pal, R, G, B, A) {pal.r = R; pal.g = G; pal.b = B; pal.a = A;}
    
    SET_COLOR(color_palette[0], 0, 0, 0, 0); // transparent
    SET_COLOR(color_palette[1], 0, 0, 0, 0x80); //pink shadow edge
    SET_COLOR(color_palette[2], 0, 0, 0xff, 0); //unknow
    SET_COLOR(color_palette[3], 0, 0, 0xff, 0); //unknow
    SET_COLOR(color_palette[4], 0, 0, 0, 0x96); //pink shadow main
    SET_COLOR(color_palette[5], 0xff, 0xff, 0, 0); //yellow aura = transparent
    SET_COLOR(color_palette[6], 0, 0, 0, 0x96); //pink aura = shadow main
    SET_COLOR(color_palette[7], 0, 0, 0, 0x80); //green aura = shadow edge
#undef SET_COLOR
#endif
    ret->size = h3def->sequencesCount;
    ret->frameSequences = (struct sequence *)malloc(sizeof(struct sequence) * ret->size);
    if (ret->frameSequences == NULL)
        goto failed;
    memset(ret->frameSequences, 0, sizeof(struct sequence) * ret->size);
    
    //calculate MAX frame number in this sprite
    frame_max_number = 0;
    i=0x310;
    for (uint32_t z=0; z<h3def->sequencesCount; z++) {
        i+=4;
        totalInBlock = readNormalNr(i,4,mem); i+=4;
        i+=8;
        i += (13 * totalInBlock);
        i += (4 * totalInBlock);
        
        frame_max_number += totalInBlock;
    }
    
    frameoffsets = (uint32_t *)malloc(frame_max_number * sizeof(uint32_t));
    if (frameoffsets == NULL)
        goto failed;
    memset(frameoffsets, 0, frame_max_number * sizeof(uint32_t));
    
    //get sequences and frames
    i=0x310;
    for (uint32_t z=0; z<h3def->sequencesCount; z++)
    {
        i+=4;
        totalInBlock = readNormalNr(i,4,mem); i+=4;
        
        i+=8;
        
        ret->frameSequences[z].size = totalInBlock;
        ret->frameSequences[z].frame_ids = (int *)malloc(sizeof(int) * totalInBlock);
        if (ret->frameSequences[z].frame_ids == NULL)
            goto failed;
        memset(ret->frameSequences[z].frame_ids, 0, sizeof(int) * totalInBlock);
        
        i += (13 * totalInBlock);
        
        for (int n=0; n<totalInBlock; n++)
        {
            uint32_t *offset = (uint32_t *)(mem + i);
            uint32_t m = 0;
            for (m=0; m<frameindex; m++) {
                if (frameoffsets[m] == *offset) {
                    ret->frameSequences[z].frame_ids[n] = m;
                    break;
                }
            }
            if (m >= frameindex) {
                ret->frameSequences[z].frame_ids[n] = frameindex;
                frameoffsets[frameindex] = *offset;
                frameindex ++;
            }
            i+=4;
        }
    }
    ret->totalFrames = frameindex;
    ret->frames = (struct decodedFrame **)malloc(sizeof(struct decodedFrame *) * frameindex);
    if (ret->frames == NULL)
        goto failed;
    memset(ret->frames, 0, sizeof(struct decodedFrame *) * frameindex);
    for (unsigned int i=0; i<frameindex; i++) {
        ret->frames[i] = decodeFrame(mem + frameoffsets[i], color_palette);
        if (ret->frames[i] == NULL) {
            goto failed;
        }
    }
    
    free(frameoffsets);
    
    return ret;
    
failed:
    if (frameoffsets != NULL) {
        free(frameoffsets);
    }

    freeSprite(ret);
    
    return NULL;
}


