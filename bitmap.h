/*

A bitmap parsing library written in pure C.
written by: pizzuhh

All information is taken from: https://en.m.wikipedia.org/wiki/BMP_file_format

*/

#ifndef BITMAP_H
#define BITMAP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

/*
* Calculate the row size.
* @param bits_per_pixel usually 25. The color depth
* @param image_width the image width
*/
#define ROW_SIZE(bits_per_pixel, image_width) (((bits_per_pixel * image_width + 31) / 32) * 4)
/*
* Calculate the image size (only the pixel data)
* @param row_size row size. Use ROW_SIZE to calculate it.
* @param height The height of the image. You can find it in BITMAPV4HEADER.bitmap_height.
*/
#define IMAGE_SIZE(row_size, height) (row_size * height)
#define S_RGB 0x42475273
#define WIN 0x206E6957



#pragma pack(push, 1)
typedef struct
{
    // File header
    uint8_t     header_field[2];
    uint32_t    size;
    uint16_t    reserved1;
    uint16_t    reserved2;
    uint32_t    offset;
} BITMAPFILEHEADER;
typedef struct {
    uint32_t        header_size;
    int32_t         bitmap_width;
    int32_t         bitmap_height;
    uint16_t        n_color_planes; // must be 1
    uint16_t        bits_per_pixel;
    uint32_t        compression_method;
    uint32_t        image_size;
    int32_t         horizontal_resolution;
    int32_t         vertical_resolution;
    uint32_t        n_colors_in_palette;
    uint32_t        important_colors;

    uint32_t        red_mask;
    uint32_t        green_mask;
    uint32_t        blue_mask;
    uint32_t        alpha_mask;
    uint32_t        color_space;
    uint32_t        color_end_points[9];
    uint32_t        red_gamma;
    uint32_t        green_gamma;
    uint32_t        blue_gamma;

} BITMAPV4HEADER;
typedef struct  {
    FILE                *file;
    BITMAPFILEHEADER    file_header;
    BITMAPV4HEADER      info_header;
    uint8_t             *pixels;

} BITMAP, *PBITMAP;
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} COLOR32BIT;
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} COLOR24BIT;
#pragma pack(pop)

#define GetImageSize(bitmap) (bitmap.info_header.image_size)

/*
* Write to bitmap_file the struct data bitmap_data
* @param bitmap_file Valid file stream to a file which is opened in write binary mode
* @param bitmap_data PBITMAP structure with valid BITMAPFILEHEADER, BITMAPV4HEADER and pixel data
*/
void WriteToBitMapFile(FILE* bitmap_file, PBITMAP bitmap_data) {
    fwrite(&bitmap_data->file_header, sizeof(bitmap_data->file_header), 1, bitmap_file);
    fwrite(&bitmap_data->info_header, sizeof(bitmap_data->info_header), 1, bitmap_file);
    fwrite(bitmap_data->pixels, bitmap_data->info_header.image_size, 1, bitmap_file);
}
/*
* Generates PBITMAP structure with valid data
* @param width the width of the bitmap file
* @param height the height of the bitmap file
* @param bits_per_pixel color depth of the bitmap file
* @param pixels UNPADED pixel data
* @return PBITMAP structure allocated by malloc() that contains valid BITMAPFILEHEADER, BITMAPV4HEADER and padded pixel data
*/
PBITMAP GenerateBitMapData(int32_t width, int32_t height, uint16_t bits_per_pixel, uint8_t *pixels, uint32_t compression) {
    uint32_t row_size = ROW_SIZE(bits_per_pixel, width);
    uint32_t image_size = IMAGE_SIZE(row_size, height);
    uint32_t size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV4HEADER) + image_size;
    
    BITMAPFILEHEADER file_header = {
        .header_field = {'B', 'M'},
        .size = size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPV4HEADER)
    };
    
    BITMAPV4HEADER info_header = {
        .header_size = sizeof(BITMAPV4HEADER),
        .bitmap_width = width,
        .bitmap_height = height,
        .n_color_planes = 1,
        .bits_per_pixel = bits_per_pixel,
        .compression_method = compression,
        .image_size = image_size,
        .horizontal_resolution = 2835,
        .vertical_resolution = 2835,
        .n_colors_in_palette = 0,
        .important_colors = 0,
        .red_mask = (bits_per_pixel == 32) ? 0x00ff0000 : 0,
        .green_mask = (bits_per_pixel == 32) ? 0x0000ff00 : 0,
        .blue_mask = (bits_per_pixel == 32) ? 0x000000ff : 0,
        .alpha_mask = (bits_per_pixel == 32) ? 0xff000000 : 0,
        .color_space = S_RGB,
        .color_end_points = {0},
        .red_gamma = 0,
        .green_gamma = 0,
        .blue_gamma = 0
    };

    PBITMAP bitmap = malloc(sizeof(BITMAP));
    bitmap->file_header = file_header;
    bitmap->info_header = info_header;

    bitmap->pixels = malloc(image_size);
    uint8_t *pixel_ptr = bitmap->pixels;
    uint32_t pixel_idx = 0;
    uint32_t pixel_size = bits_per_pixel / 8;

    for (int y = 0; y < height; ++y) {
        memcpy(pixel_ptr, &pixels[pixel_idx], width * pixel_size);
        pixel_ptr += width * pixel_size;
        pixel_idx += width * pixel_size;
        uint32_t padding_size = row_size - width * pixel_size;
        memset(pixel_ptr, 0, padding_size);
        pixel_ptr += padding_size;
    }

    return bitmap;
}

// compression enum

typedef enum  {
    BI_RGB = 0,
    BI_RLE8 = 1,
    BI_RLE4 = 2,
    BI_BITFIELDS = 3,
    BI_JPEG = 4,
    BI_PNG = 5,
    BI_ALPHABITFIELDS = 6,
    BI_CMYK = 11,
    BI_CMYKRLE8 = 12,
    BI_CMYKRLE4 = 13
} COMPRESSION;

/*
* Creates a bitmap image file.
* @param file_name the path to the output file
* @param width the width of the bitmap file
* @param height the height of the bitmap file
* @param pixels UNPADED pixel data
* @return A BITMAP struct that contains the headers, pixel data and a file stream
*/
BITMAP CreateBitMap(const char* file_name, int32_t width, int32_t height, uint8_t *pixels, uint32_t color_depth, COMPRESSION compression) {
    PBITMAP bitmap_data = GenerateBitMapData(width, height, color_depth, pixels, compression);
    FILE *bitmap_file = fopen(file_name, "wb+");
    WriteToBitMapFile(bitmap_file, bitmap_data);
    fseek(bitmap_file, 0, SEEK_SET);
    bitmap_data->file = bitmap_file;
    return *bitmap_data;
}
/*
* Prints bitmap header values
* @param f A valid file pointer to a bitmap file
*/
void PrintBitMapInfo(FILE *f)
{
    BITMAPFILEHEADER file_header;
    BITMAPV4HEADER info_header;
    fread(&file_header, sizeof(file_header), 1, f);
    fread(&info_header, sizeof(info_header), 1, f);
    printf("----BEGIN FILE HEADER---");
    printf("Size: %d\n", file_header.size);
    printf("Reserved 1: %d\n", file_header.reserved1);
    printf("Reserved 2: %d\n", file_header.reserved2);
    printf("Pixel array offset: %d\n", file_header.offset);
    printf("----END FILE HEADER---\n");
    printf("----START BITMAPV4HEADER HEADER---\n");
    printf("Header size: %d\n", info_header.header_size);
    printf("Width: %d\n", info_header.bitmap_width);
    printf("Height: %d\n", info_header.bitmap_height);
    printf("Planes: %d\n", info_header.n_color_planes);
    printf("Color depth: %d\n", info_header.bits_per_pixel);
    printf("Compression: %d\n", info_header.compression_method);
    printf("Image Size: %d\n", info_header.image_size);
    printf("Horizontal Resolution: %d\n", info_header.horizontal_resolution);
    printf("Vertical Resolution: %d\n", info_header.vertical_resolution);
    printf("Palette: %d\n", info_header.n_colors_in_palette);
    printf("Red Mask: %X\n", info_header.red_mask);
    printf("Green Mask: %X\n", info_header.green_mask);
    printf("Blue Mask: %X\n", info_header.blue_mask);
    printf("Alpha Mask: %X\n", info_header.alpha_mask);
    printf("Color space: %d\n", info_header.color_space);
    printf("Red Gamma: %d\n", info_header.red_gamma);
    printf("Green Gamma: %d\n", info_header.green_gamma);
    printf("Blue Gamma: %d\n", info_header.blue_gamma);
    printf("----END BITMAPV4HEADER HEADER---\n");
}
/*
* Reads a bitmap file.
* @param file_name the path to a bitmap file
* @return BITMAP structure which contains the header and pixel data of the bitmap file.
*/
BITMAP ReadBitMap(const char *file_name)
{
    BITMAP bitmap;
    FILE *bitmap_file = fopen(file_name, "rb");
    fread(&bitmap.file_header, sizeof(bitmap.file_header), 1, bitmap_file);
    fread(&bitmap.info_header, sizeof(bitmap.info_header), 1, bitmap_file);
    uint32_t row_size = ROW_SIZE(bitmap.info_header.bits_per_pixel, bitmap.info_header.bitmap_width);
    uint32_t pixel_size = bitmap.info_header.bits_per_pixel / 8;
    uint32_t padded_row_size = (bitmap.info_header.bitmap_width * pixel_size + 3) & ~3; // Adjusted for padding
    fseek(bitmap_file, bitmap.file_header.offset, SEEK_SET);
    bitmap.pixels = malloc(bitmap.info_header.image_size);
    uint8_t *ptr = bitmap.pixels;
    for (int y = 0; y < bitmap.info_header.bitmap_height; y++)
    {
        fread(ptr, 1, padded_row_size, bitmap_file); // Read the whole row including padding
        fseek(bitmap_file, row_size - padded_row_size, SEEK_CUR); // Move past the padding
        ptr += bitmap.info_header.bitmap_width * pixel_size;
    }
    fclose(bitmap_file);
    return bitmap;
}

void cleanup(PBITMAP bmp) {
    if (bmp == NULL) {
        fprintf(stderr, "bmp is NULL. Not freeing!\n");
        return;
    }

    if (bmp->pixels) {
        free(bmp->pixels);
        bmp->pixels = NULL;
    } else {
        fprintf(stderr, "bmp->pixels is NULL. Not freeing!\n");
    }
    
    if (bmp->file) {
        fclose(bmp->file);
        bmp->file = NULL;
    } else {
        fprintf(stderr, "bmp->file is NULL. Not closing!\n");
    }
    bmp = NULL;
}


/*
* A function that inverts the pixels
* @param pixels the input pixel array
* @param image_size bitmap image size. Can be obtained by using the IMAGE_SIZE and ROW_SIZE macros
* @return the inverted pixel values are returned via pixels argument. If you want to keep 
* the original pixel data make sure to save it!
*/
void InvertPixel(uint8_t *pixels, uint32_t image_size) {
    for (int i = 0; i < image_size; i += 4) {
        pixels[i] = 255 - pixels[i];         // Blue component
        pixels[i + 1] = 255 - pixels[i + 1]; // Green component
        pixels[i + 2] = 255 - pixels[i + 2]; // Red component
    }
}

void SetPixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, PBITMAP file) {
    uint32_t index = (y * file->info_header.bitmap_width + x) * 4;
    file->pixels[index]     = blue;                     // Blue
    file->pixels[index + 1] = green;                    // Green
    file->pixels[index + 2] = red;                      // Red
    file->pixels[index + 3] = alpha;                    // Alpha
}

#endif
