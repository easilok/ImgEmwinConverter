/**
 * Based on: 
 * png2c
 * Oleg Vaskevich, Northeastern University
 * 4/17/2013
 *
 * Program to convert a PNG file to an C header as an array
 * in either RGB565 or RGB5A1 format. This is useful for
 * embedding pixmaps to display with a PAL board or Arduino.
 *
 * Thanks to: http://zarb.org/~gc/html/libpng.html
 *            http://stackoverflow.com/a/2736821/832776
 */
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include <math.h>       /* round, floor, ceil, trunc */

/* #define TEST_OUPUT 1 */


FILE *file;

int x, y;

int width, height;
png_byte colorType;
png_byte bitDepth;

png_structp pPng;
png_infop pInfo;
int numPasses;
png_bytep *rows;

void toRGBA565 (unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, unsigned char * pixelExport) {
	unsigned char redConverted = 0;
	unsigned char greenConverted = 0;
	unsigned char blueConverted = 0;

	redConverted = round(red * 31.0f / 255);
	greenConverted = round(green * 63.0f / 255);
	blueConverted = round(blue * 31.0f / 255);

	pixelExport[0] = 255 - alpha;
#ifdef USE_STD_RGB
	pixelExport[1] = (redConverted << (8 - 5)) & 0b11111000;
	pixelExport[1] |= (greenConverted >> (6 - (8 - 5))) & 0b111;

	pixelExport[2] = ((greenConverted & 0b111) << (8-3)) & 0b11100000;
	pixelExport[2] |= (blueConverted & 0b11111);
#else
	pixelExport[1] = (blueConverted << 3) & 0b11111000;
	pixelExport[1] |= (greenConverted >> 3) & 0b111;

	pixelExport[2] = ((greenConverted & 0b111) << 5) & 0b11100000;
	pixelExport[2] |= (redConverted & 0b11111);
#endif
}

int main(int argc, char* argv[]) {
    // check parameter
    if (argc != 2 && argc != 3) {
        printf("Usage: NuriaEmwinConverter [PNG file] \n"
                "A source file for project include will be generated in the same directory\n"
                "A raw file to pass in an external drive will be generated in the same folder\n");
        return -1;
    }
    
    // open file
    printf("Opening file %s...\n", argv[1]);
    unsigned char header[8];
    file = fopen(argv[1], "rb");
    if (!file) {
        printf("Could not open file %s.\n", argv[1]);
        return -1;
    }
    
    // read the header
    printf("Reading header...\n");
    if (fread(header, 1, 8, file) <= 0) {
        printf("Could not read PNG header.\n");
        return -1;
    }

    // make sure it's a PNG file
    if (png_sig_cmp(header, 0, 8)) {
        printf("File is not a valid PNG file.\n");
        return -1;
    }
    printf("Valid PNG file found. Reading more...\n");
    
    // create the read struct
    pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pPng) {
        printf("Error: couldn't read PNG read struct.\n");
        return -1;
    }

    // create the info struct
    png_infop pInfo = png_create_info_struct(pPng);
    if (!pInfo) {
        printf("Error: couldn't read PNG info struct.\n");
        png_destroy_read_struct(&pPng, (png_infopp)0, (png_infopp)0);
        return -1;
    }
    
    // basic error handling
    if (setjmp(png_jmpbuf(pPng))) {
        printf("Error reading PNG file properties.\n");
        return -1;
    }
    
    // read PNG properties
    png_init_io(pPng, file);
    png_set_sig_bytes(pPng, 8);
    png_read_info(pPng, pInfo);
    width = png_get_image_width(pPng, pInfo);
    height = png_get_image_height(pPng, pInfo);
    colorType = png_get_color_type(pPng, pInfo);
    bitDepth = png_get_bit_depth(pPng, pInfo);
    numPasses = png_set_interlace_handling(pPng);
    png_read_update_info(pPng, pInfo);
    printf("Metadata: w: %i, h: %i, color type: %i, bit depth: %i, num passes: %i\n", width, height, colorType, bitDepth, numPasses);

    // read the file
    if (setjmp(png_jmpbuf(pPng))) {
        printf("Error reading PNG file data.\n");
        return -1;
    }
    rows = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y = 0; y < height; y++)
        rows[y] = (png_byte*) malloc(png_get_rowbytes(pPng,pInfo));
    png_read_image(pPng, rows);
    fclose(file);
    
    // verify that the PNG image is RGBA
    if (png_get_color_type(pPng, pInfo) != PNG_COLOR_TYPE_RGBA) {
        printf("Error: Input PNG file must be RGBA (%d), but is %d.", PNG_COLOR_TYPE_RGBA, png_get_color_type(pPng, pInfo));
        return -1;
    }
    
    // get the name of the image, without the extension (assume it ends with .png)
    size_t len = strlen(argv[1]);
    char *imageName = malloc(len - 3);
    memcpy(imageName, argv[1], len - 4);
    imageName[len - 4] = 0;
    printf("Creating output file %s.h...\n", imageName);
                       
    // create output file
    FILE *outputSource, *outputPixel, *outputRaw;
    char outputSourceName[100], outputPixelName[100], outputRawName[100];
    sprintf(outputSourceName, "%s.c", imageName);
    sprintf(outputPixelName, "%s.txt", imageName);
    sprintf(outputRawName, "%s.raw", imageName);
    outputSource = fopen(outputSourceName, "w"); // overwrite/create empty file
    outputPixel = fopen(outputPixelName, "w"); // overwrite/create empty file
    outputRaw = fopen(outputRawName, "wb"); // overwrite/create empty file
    
#ifndef TEST_OUPUT
    // write the source
		fprintf(outputSource,
				"// Generated by png2c [(c) Oleg Vaskevich 2013 - Edited lcp]\n\n"
				"#include <stdlib.h>\n\n"
				"#include \"GUI.h\"\n\n"
				"#ifndef GUI_CONST_STORAGE\n"
				"	#define GUI_CONST_STORAGE const\n"
				"#endif\n\n"
				"extern GUI_CONST_STORAGE GUI_BITMAP bm%s;\n\n"
				"IN_EXTERNAL_FLASH static GUI_CONST_STORAGE unsigned char _ac%s[] = {\n",
				imageName, imageName);
#endif

    // go through each pixel
    printf("Processing pixels...\n");
    for (y=0; y<height; y++) {
        png_byte* row = rows[y];
        for (x=0; x<width; x++) {
            png_byte* pixel = &(row[x*4]);
						fprintf(outputPixel, "Red: %d, Green:%d, Blue:%d, Alpha:%d\n", pixel[0], pixel[1], pixel[2], pixel[3]);
						unsigned char convPixel[3] = {0};
						toRGBA565(pixel[0], pixel[1], pixel[2], pixel[3], convPixel);
						// the last pixel shouldn't have a comma
#ifndef TEST_OUPUT
						if (y == height-1 && x == width-1)
								fprintf(outputSource, "0x%X, 0x%X, 0x%X\n};\n", convPixel[0], convPixel[1], convPixel[2]);
						else
								fprintf(outputSource, "0x%X, 0x%X, 0x%X, ", convPixel[0], convPixel[1], convPixel[2]);
#else

						fprintf(outputSource, "0x%X, 0x%X, 0x%X\n", convPixel[0], convPixel[1], convPixel[2]);
#endif
						fwrite(convPixel, sizeof(convPixel), 1, outputRaw);
        }
#ifndef TEST_OUPUT
        fprintf(outputSource, "\n");
#endif
    }
#ifndef TEST_OUPUT
    // write the source
		fprintf(outputSource,
				"\nGUI_CONST_STORAGE GUI_BITMAP bm%s = {\n"
				"		%d, // xSize\n"
				"		%d, // ySize\n"
				"		%d, // BytesPerLine\n"
				"		%d, // BitsPerPixel\n"
				"		(unsigned char *)_ac%s, // Pointer to picture data\n"
				"		NULL, // Pointer to palette\n"
				"		GUI_DRAW_BMPA565\n"
				"};\n\n",
				imageName, width, height, width * (24/8), 24 /*bitDepth*/, imageName);
#endif
    fclose(outputSource);
    fclose(outputPixel);
    printf("Done! Generated %s.h, %s.c and %s.raw.\n", imageName, imageName, imageName);

    return 0;
}



