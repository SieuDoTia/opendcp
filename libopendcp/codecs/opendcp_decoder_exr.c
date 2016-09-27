/* opendcp_decoder_exr.c */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "opendcp.h"
#include "opendcp_image.h"

#define MAGIC_NUMBER 0x762f3101

typedef enum {
    BMP_TOP         = 0,
    BMP_BOTTOM      = 1,
} line_order;

typedef enum {
    EXR_NO       = 0,          /* no compression                  */
    EXR_RLE      = 1,          /* 8-bit run-length-encoded (not supported) */
    EXR_ZIPS     = 2,          /* zip single line (not supported) */
    EXR_ZIP      = 3,          /* zip 16 lines                    */
    EXR_PIZ      = 4,          /* piz (not supported)             */
    EXR_PXR24    = 5           /* pixar 24 bit (not supported)    */
    EXR_B44      = 6           /* b44 (not supported)             */
    EXR_B44A     = 7           /* b44a (not supported)            */
    EXR_DWAA     = 8            /* dwaa 32 lines (not supported)  */
    EXR_DWAB     = 9            /* dwab 256 lines (not supported) */
} exr_compression_enum;

typedef struct {
    uint32_t magic_num;         /* magic number 0x762f3101        */
} exr_magic_num_t;

typedef struct {
    uint16 num_channel;         /* magic number 0x762f3101        */
} exr_channel_list;

typedef struct {
    int32_t  width;              /* width of image                */
    int32_t  height;             /* height of image               */
} exr_image_header_t;

typedef struct {
    exr_image_header_t       image;
    int                      row_order;
} exr_image_t;

/* for change half become float */
union {
    unsigned int i;
    unsigned float f;
} u_intfloat;

float half2float( unsigned short half ) {
    
    u_intfloat int_fl;

    // normal number +
    if( (half > 0x03ff) && (half < 0x7c00) ) {
       unsigned int exponent = ((half & 0x7c00) + 112) << 23;
       unsigned int value = (half & 0x03ff) << 13;
       inf_fl.i = exponent | value;
    }
     // normal number -
    else if( (half > 0x83ff) && (half < 0xfc00) ) {
       unsigned int exponent = ((half & 0x7c00) + 112) << 23;
       unsigned int value = (half & 0x03ff) << 13;
       inf_fl.i = 0x80000000 + exponent + value;
    }
    // not normal number +
    else if( (half > 0x0000) && (half < 0x0400) ) {
       unsigned int exponent = ((half & 0x7c00) + 102) << 23;
       unsigned int value = (half & 0x03ff) << 13;
      inf_fl.i = exponent + value;
    }
    // not normal number -
    else if( (half > 0x8000) && (half < 0x8400) ) {
       unsigned int exponent = ((half & 0x7c00) + 102) << 23;
       unsigned int value = (half & 0x03ff) << 13;
      inf_fl.i = 0x80000000 + exponent + value;
    }
    // zero +
    else if( half == 0x0000 )
        int_fl.i = 0x00000000;
 
    // zero -
    else if( half == 0x8000 )
        int_fl.i = 0x80000000;
 
    // infinity +
    else if( half == 0x7c00 ) 
        int_fl.i = 0x7f800000;
  
    // infinity -
    else if( half == 0xfc00 ) {
        int_fl.i = 0xff800000;

    // NaN +
    else if( (half > 0x7c00) && (half < 0x8000) ) 
        int_fl.i = ((half & 0x3ff) << 13) | 0x7f800000;
 
    // NaN -
    else if( half > 0xfc00 )
        int_fl.i = ((half & 0x3ff) << 13) | 0xff800000;

    return int_fl.f;
}