/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};

/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
	/* function edited for the implementation of the octree coloring */
	photo_t *p = room_photo(r);
	set_palette_octree(p->palette);
    cur_room = r;
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}

/*OCTREE STRUCT*/

typedef struct oct_node{
	uint32_t RGB;
	uint32_t count;
	/* [0]SUMR, [1]SUMG, [2]SUMB*/
	uint32_t sumrgb[3];
	uint32_t r_g_b_index;
}oct_node;

typedef struct Octree{
	oct_node second_layer[SECOND_LAYER_SIZE]; 
	oct_node fourth_layer[FOURTH_LAYER_SIZE]; 
}Octree;

/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB. You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */



    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

	Octree tree;
	int i;
	/* Initialize second layer of Octree*/
	for(i=0; i < SECOND_LAYER_SIZE; i++){
		tree.second_layer[i].count = 0;
		tree.second_layer[i].sumrgb[RED_INDEX] = 0;  
		tree.second_layer[i].sumrgb[GREEN_INDEX] = 0; 
		tree.second_layer[i].sumrgb[BLUE_INDEX] = 0; 
		tree.second_layer[i].RGB = i;
	}
	/* Initialize fourth layer of Octree*/
	for(i=0; i < FOURTH_LAYER_SIZE; i++){
		tree.fourth_layer[i].count = 0;
		tree.fourth_layer[i].sumrgb[RED_INDEX] = 0;
		tree.fourth_layer[i].sumrgb[GREEN_INDEX] = 0;
		tree.fourth_layer[i].sumrgb[BLUE_INDEX] = 0;
		tree.fourth_layer[i].RGB = i;
	}

	uint32_t complex_color;
	uint32_t simple_color;
	uint32_t Red;
	uint32_t Green;
	uint32_t Blue;
	uint32_t fourth_layer_idx;
	/*creat an array contains each pixel. */
	uint32_t img_array[p->hdr.width*p->hdr.height];
   
    for (y = p->hdr.height; y-- > 0; ) {
	/* Loop over columns */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;
	    }

		/* store each pixel*/
		img_array[p->hdr.width * y + x] = pixel;

		/* convert the pixel color from 16 to 12*/
		int temp_Red = (pixel >> 12) & 0x0F; //mask for the first four bits 
		int temp_Green = (pixel >> 7) & 0x0F;
		int temp_Blue = (pixel >> 1) & 0x0F;
		temp_Red <<= 8;
		temp_Green <<= 4;
		complex_color = (temp_Red|temp_Green|temp_Blue);

		Red = ((pixel >> 11) & 0x1F)<<1;
		Green = (pixel >> 5) & 0x3F;
		Blue = (pixel & 0x1F)<<1;

		/* Update the data in the Octree*/
		tree.fourth_layer[complex_color].count +=1;
		tree.fourth_layer[complex_color].sumrgb[RED_INDEX] += Red;
		tree.fourth_layer[complex_color].sumrgb[GREEN_INDEX] += Green;
		tree.fourth_layer[complex_color].sumrgb[BLUE_INDEX] += Blue;


	    /* 
	     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
	     * and 6 bits blue).  We change to 2:2:2, which we've set for the
	     * game objects.  You need to use the other 192 palette colors
	     * to specialize the appearance of each photo.
	     *
	     * In this code, you need to calculate the p->palette values,
	     * which encode 6-bit RGB as arrays of three uint8_t's.  When
	     * the game puts up a photo, you should then change the palette 
	     * to match the colors needed for that photo.
	     */
		/*We don't need the old color anymore since we have an optimized palette*/
	    // p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
		// 			    (((pixel >> 9) & 0x3) << 2) |
		// 			    ((pixel >> 3) & 0x3));
	}
    }

	/* use quick sort method to sort the L4 level*/
	qsort((oct_node*)tree.fourth_layer,FOURTH_LAYER_SIZE,sizeof(tree.fourth_layer[0]),comparator);

	uint32_t second_layer_idx;
	for (i=0; i<FOURTH_LAYER_SIZE;i++){
		/* Record the index after the quick sort. */
		tree.fourth_layer[tree.fourth_layer[i].RGB].r_g_b_index = i;
		/* Set up the first 128 color of the palette (L4)*/
		if(i < NUM_FOURTH_LAYER_COLORS){
			/* If the color is not been used, then continue*/
			if (tree.fourth_layer[i].count ==0){
				break;
			}
			p->palette[i][0] = tree.fourth_layer[i].sumrgb[RED_INDEX] / tree.fourth_layer[i].count;
			p->palette[i][1] = tree.fourth_layer[i].sumrgb[GREEN_INDEX] / tree.fourth_layer[i].count;
			p->palette[i][2] = tree.fourth_layer[i].sumrgb[BLUE_INDEX] / tree.fourth_layer[i].count;
		}
		/* Use the remaining L4 color to set up L2*/
		else{
			/* Get the first 4 bits for each color*/
			int temp_Red = (tree.fourth_layer[i].RGB >> 10) & 0x03; //mask for the last 2 bits
			int temp_Green = (tree.fourth_layer[i].RGB >> 6) & 0x03;
			int temp_Blue = (tree.fourth_layer[i].RGB >> 2) & 0x03;
			temp_Red <<= 4;
			temp_Green <<= 2; 
			second_layer_idx = (temp_Red|temp_Green|temp_Blue);
			tree.second_layer[second_layer_idx].sumrgb[RED_INDEX] += tree.fourth_layer[i].sumrgb[RED_INDEX];
			tree.second_layer[second_layer_idx].sumrgb[GREEN_INDEX] += tree.fourth_layer[i].sumrgb[GREEN_INDEX];
			tree.second_layer[second_layer_idx].sumrgb[BLUE_INDEX] += tree.fourth_layer[i].sumrgb[BLUE_INDEX];
			tree.second_layer[second_layer_idx].count += tree.fourth_layer[i].count;
		}
	}

	/* Set up the last 64 color of the palette (L2)*/
	for (i=0; i<SECOND_LAYER_SIZE; i++){
		/* If the color is not been used, then continue*/
		if(tree.second_layer[i].count ==0 ){
			continue;
		}
		p->palette[i+NUM_FOURTH_LAYER_COLORS][0] = tree.second_layer[i].sumrgb[RED_INDEX] / tree.second_layer[i].count;
		p->palette[i+NUM_FOURTH_LAYER_COLORS][1] = tree.second_layer[i].sumrgb[GREEN_INDEX] / tree.second_layer[i].count;
		p->palette[i+NUM_FOURTH_LAYER_COLORS][2] = tree.second_layer[i].sumrgb[BLUE_INDEX] / tree.second_layer[i].count;
	}

	for (y = p->hdr.height; y-- > 0; ) {
		/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) {
			/*get the pixel we stored earlier*/
			pixel = img_array[p->hdr.width * y + x];

			/* convert the pixel into 12 and 6 bit color type*/
			int temp_Red = (pixel >> 12) & 0x0F; //mask for the first four bits 
			int temp_Green = (pixel >> 7) & 0x0F;
			int temp_Blue = (pixel >> 1) & 0x0F;
			temp_Red <<= 8;
			temp_Green <<= 4;
			complex_color = (temp_Red|temp_Green|temp_Blue);

			temp_Red = (complex_color >> 10) & 0x03; //mask for the last 2 bits
			temp_Green = (complex_color >> 6) & 0x03;
			temp_Blue = (complex_color >> 2) & 0x03;
			temp_Red <<= 4;
			temp_Green <<= 2; //bit shifting is to get the R-G-B fields into the correct positions 
			simple_color = (temp_Red|temp_Green|temp_Blue);

			fourth_layer_idx = tree.fourth_layer[complex_color].r_g_b_index;

			/* If the index of the color is less than 128, we need find the color
				in the range of 64 ~ 193, otherwise, find in the range of 194 ~ 255*/
			if(fourth_layer_idx < NUM_FOURTH_LAYER_COLORS){
				p->img[p->hdr.width * y + x] = (char)(64+fourth_layer_idx);
			}else{
				p->img[p->hdr.width * y + x] = (char)(64+NUM_FOURTH_LAYER_COLORS+simple_color);
			}
		}
	}

    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}


/* comparator for qsort that tests to see if p > q*/
int comparator(const void *p, const void *q){
	int p_count = ((oct_node*)p) -> count;
	int q_count = ((oct_node*)q) -> count;
	return (q_count - p_count);
}
