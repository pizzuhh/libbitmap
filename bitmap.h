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

/*
* Calculate the row size.
* @param bits_per_pixel usually 25. The color depth
* @param image_width the image width
*/
#define ROW_SIZE(bits_per_pixel, image_width) (((bits_per_pixel * image_width + 31) / 32) * 4)
/*
* Calculate the image size (only the pixel data)
* @param row_size row size. Use ROW_SIZE to calculate it.
* @param height The height of the image. You can find it in BITMAPINFOHEADER.bitmap_height.
*/
#define IMAGE_SIZE(row_size, height) (row_size * height)
#pragma pack(push, 1)
typedef struct
{
    // File header
    unsigned char   header_field[2];
    unsigned int    size;
    unsigned short  reserved1;
    unsigned short  reserved2;
    unsigned int    offset;
} BITMAPFILEHEADER;
typedef struct {
    unsigned    int     header_size;
    signed      int     bitmap_width;
    signed      int     bitmap_height;
    unsigned    short   n_color_planes; // must be 1
    unsigned    short   bits_per_pixel;
    unsigned    int     compression_method;
    unsigned    int     image_size;
    signed      int     horizontal_resolution;
    signed      int     vertical_resolution;
    unsigned    int     n_colors_in_palette;
    unsigned    int     important_colors;

    unsigned    int     red_mask;
    unsigned    int     green_mask;
    unsigned    int     blue_mask;
    unsigned    int     alpha_mask;

} BITMAPINFOHEADER;
typedef struct  {
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    unsigned char *pixels;

} BITMAP, *PBITMAP;

#pragma pack(pop)

/*
* Write to bitmap_file the struct data bitmap_data
* @param bitmap_file Valid file pointer to a file which is opened in write binary mode
* @param bitmap_data PBITMAP structure with valid BITMAPFILEHEADER, BITMAPINFOHEADER and pixel data
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
* @return PBITMAP structure allocated by malloc() that contains valid BITMAPFILEHEADER, BITMAPINFOHEADER and padded pixel data
*/
PBITMAP GenerateBitMapData(signed int width, signed int height, unsigned short bits_per_pixel, unsigned char *pixels, unsigned int compression) {
    unsigned int size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 4;
    BITMAPFILEHEADER file_header = {
        .header_field = {'B', 'M'},
        .size = size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
    };
    BITMAPINFOHEADER info_header = {
        .header_size = 40,
        .bitmap_width = width,
        .bitmap_height = height,
        .n_color_planes = 1,
        .bits_per_pixel = bits_per_pixel,
        .compression_method = compression,
        .image_size =  IMAGE_SIZE(ROW_SIZE(bits_per_pixel, width), height),
        .horizontal_resolution = 2835,
        .vertical_resolution = 2835,
        .n_colors_in_palette = 0,
        .important_colors = 0,
        .red_mask = (bits_per_pixel == 32)      ?   0x0000FF00 : 0,
        .green_mask = (bits_per_pixel == 32)    ?   0x00FF0000 : 0,
        .blue_mask = (bits_per_pixel == 32)     ?   0xFF000000 : 0,
        .alpha_mask = (bits_per_pixel == 32)    ?   0x000000FF : 0
    };
    PBITMAP bitmap = malloc(sizeof(BITMAP));
    bitmap->file_header = file_header;
    bitmap->info_header = info_header;

    unsigned int row_size = ROW_SIZE(bitmap->info_header.bits_per_pixel, width);
    bitmap->pixels = malloc(info_header.image_size);
    unsigned char *pixel_ptr = bitmap->pixels;
    unsigned int pixel_idx = 0;
    unsigned int pixel_size = bits_per_pixel / 8;
    for (int y = 0; y < height; ++y) {
        memcpy(pixel_ptr, &pixels[pixel_idx], width * pixel_size);
        pixel_ptr += pixel_size;
        pixel_idx += pixel_size;
        unsigned int padding_size = row_size - width * pixel_size;
        memset(pixel_ptr, 0, padding_size);
        pixel_ptr += padding_size;
    }
    return bitmap;
}
/*
* Creates a bitmap image file.
* @param file_name the path to the output file
* @param width the width of the bitmap file
* @param height the height of the bitmap file
* @param pixels UNPADED pixel data
* @return A file pointer to the newly created bitmap file. fseek() is ran automatically to the beginning of the file.
*/
FILE *CreateBitMapFile(const char* file_name, signed int width, signed int height, unsigned char *pixels) {
    PBITMAP bitmap_data = GenerateBitMapData(width, height, 32, pixels, 3);
    FILE *bitmap_file = fopen(file_name, "wb+");
    WriteToBitMapFile(bitmap_file, bitmap_data);
    free(bitmap_data);
    fseek(bitmap_file, 0, SEEK_SET);
    return bitmap_file;
}
/*
* Prints bitmap header values
* @param f A valid file pointer to a bitmap file
*/
void PrintBitMapInfo(FILE *f)
{
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    fread(&file_header, sizeof(file_header), 1, f);
    fread(&info_header, sizeof(info_header), 1, f);
    printf("----BEGIN FILE HEADER---");
    printf("Size: %d\n", file_header.size);
    printf("Reserved 1: %d\n", file_header.reserved1);
    printf("Reserved 2: %d\n", file_header.reserved2);
    printf("Pixel array offset: %d\n", file_header.offset);
    printf("----END FILE HEADER---\n");
    printf("----START BITMAPINFO HEADER---\n");
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
    printf("Important Colors: %d\n", info_header.important_colors);
    printf("----END BITMAPINFO HEADER---\n");
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
    if (bitmap.info_header.compression_method != 0) assert("Not implemented");
    unsigned int row_size = ROW_SIZE(bitmap.info_header.bits_per_pixel, bitmap.info_header.bitmap_width);
    fseek(bitmap_file, bitmap.file_header.offset, SEEK_SET);
    bitmap.pixels = malloc(bitmap.info_header.bitmap_height * bitmap.info_header.bitmap_width * 3);
    unsigned char *ptr = bitmap.pixels;
    for (int y = 0; y < bitmap.info_header.bitmap_height; y++)
    {
        fread(ptr, 1, bitmap.info_header.bitmap_width * 3, bitmap_file);
        fseek(bitmap_file, row_size - bitmap.info_header.bitmap_width * 3, SEEK_CUR);
        ptr += bitmap.info_header.bitmap_width * 3;
    }
    fclose(bitmap_file);
    return bitmap;
}
/*
* A function that inverts the pixels
* @param pixels the input pixel array
* @param image_size bitmap image size. Can be obtained by using the IMAGE_SIZE and ROW_SIZE macros
* @return the inverted pixel values are returned via pixels argument. If you want to keep 
* the original pixel data make sure to save it!
*/
void invert_pixel(unsigned char *pixels, unsigned int image_size) {
    for (int i = 0; i < image_size; i += 3) {
        pixels[i] = 255 - pixels[i];         // Blue component
        pixels[i + 1] = 255 - pixels[i + 1]; // Green component
        pixels[i + 2] = 255 - pixels[i + 2]; // Red component
    }
}
#endif
