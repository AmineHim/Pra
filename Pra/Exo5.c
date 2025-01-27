#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <omp.h>
#include <sys/time.h>


int NB_THREADS = 0;

typedef struct color_pixel_struct {
    unsigned char r,g,b;
} color_pixel_type;

typedef struct color_image_struct
{
  int width, height;
  color_pixel_type * pixels;
} color_image_type;

typedef struct grey_image_struct
{
  int width, height;
  unsigned char * pixels;
} grey_image_type;


/**********************************************************************/

color_image_type * loadColorImage(char *filename){
  int i, width,height,max_value;
  char format[8];
  color_image_type * image;
  FILE * f = fopen(filename,"r");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fscanf(f,"%s\n",format);
  assert( (format[0]=='P' && format[1]=='3'));  // check P3 format
  while(fgetc(f)=='#') // commentaire
    {
      while(fgetc(f) != '\n'); // aller jusqu'a la fin de la ligne
    }
  fseek( f, -1, SEEK_CUR);
  fscanf(f,"%d %d\n", &width, &height);
  fscanf(f,"%d\n", &max_value);
  image = malloc(sizeof(color_image_type));
  assert(image != NULL);
  image->width = width;
  image->height = height;
  image->pixels = malloc(width*height*sizeof(color_pixel_type));
  assert(image->pixels != NULL);

  for(i=0 ; i<width*height ; i++){
      int r,g,b;
      fscanf(f,"%d %d %d", &r, &g, &b);
      image->pixels[i].r = (unsigned char) r;
      image->pixels[i].g = (unsigned char) g;
      image->pixels[i].b = (unsigned char) b;
    }
  fclose(f);
  return image;
}

/**********************************************************************/

grey_image_type * createGreyImage(int width, int height){
  grey_image_type * image = malloc(sizeof(grey_image_type));
  assert(image != NULL);
  image->width = width;
  image->height = height;
  image->pixels = malloc(width*height*sizeof(unsigned char));
  assert(image->pixels != NULL);
  return(image);
}

/**********************************************************************/

void saveGreyImage(char * filename, grey_image_type *image){
  int i;
  FILE * f = fopen(filename,"w");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fprintf(f,"P2\n%d %d\n255\n",image->width,image->height);
  for(i=0 ; i<image->width*image->height ; i++){
    fprintf(f,"%d\n",image->pixels[i]);
  }
  fclose(f);
}

/**********************************************************************/

void saveColorImage(char * filename, color_image_type *image){
  int i;
  FILE * f = fopen(filename,"w");
  if (!f){
    fprintf(stderr,"Cannot open file %s...\n",filename);
    exit(-1);
  }
  fprintf(f,"P3\n%d %d\n255\n",image->width,image->height);
  for(i=0 ; i<image->width*image->height ; i++){
    fprintf(f,"%d\n%d\n%d\n",image->pixels[i].r, image->pixels[i].g, image->pixels[i].b);
  }
  fclose(f);
}

/**********************************************************************/

void colorToGrey(color_image_type *col_img, grey_image_type *grey_img){
    for (int i=0; i < col_img->height ; i++)
      for (int j=0; j < col_img->width ; j++){
        int index = i * col_img->width + j;
        color_pixel_type *pix = &(col_img->pixels[index]);
        grey_img->pixels[index] = (299*pix->r + 587*pix->g + 114*pix->b)/1000;
      }
}

/**********************************************************************/

int* histogram(grey_image_type* image) {
    int* hist = calloc(256, sizeof(int));

    #pragma omp parallel num_threads(NB_THREADS)
    {
        int* private_hist = calloc(256, sizeof(int));

        #pragma omp for reduction(+:hist[:256])
        for (int i = 0; i < image->width * image->height; i++) {
            private_hist[image->pixels[i]]++;
        }

        for (int j = 0; j < 256; j++) {
            hist[j] += private_hist[j];
        }

        free(private_hist);
    }

    return hist;
}

/**********************************************************************/

int * cumulative_histogram(int *hist){
    int *cumulative_hist = calloc(256, sizeof(int));
    cumulative_hist[0] = hist[0];
    for (int i = 1; i < 256; i++){
        cumulative_hist[i] = cumulative_hist[i-1] + hist[i];
    }
    return cumulative_hist;
}

/**********************************************************************/

void equalizeGrey(grey_image_type *img) {
    int i, j;
    int * hist = malloc(sizeof(int) * 256);
    int * cum_hist = malloc(sizeof(int) * 256);
    int S = img->width * img->height;
    double t = 0, start = 0, stop = 0;
    start = omp_get_wtime();
    hist = histogram(img);
    cum_hist = cumulative_histogram(hist);
    #pragma omp parallel for num_threads(NB_THREADS)
    for (i = 0; i < S; i++) {
        img->pixels[i] = (unsigned char) (255 * cum_hist[img->pixels[i]] / S);
    }
    stop=omp_get_wtime();
    t=stop-start;
    printf("Avec %d threads c'est fini en %f\n",NB_THREADS, t);
    NB_THREADS *= 2;
}

/**********************************************************************/
int main(int argc, char ** argv){
    color_image_type * col_img;
    grey_image_type * grey_img;

    if (argc != 3){
        printf("Usage: togrey <input image> <output image>\n");
        exit(-1);
    }
    char *input_file = argv[1];
    char *output_file = argv[2];

    col_img = loadColorImage(input_file);
    grey_img = createGreyImage(col_img->width, col_img->height);
    NB_THREADS = 4;
    colorToGrey(col_img, grey_img);
    equalizeGrey(grey_img);
    saveGreyImage(output_file, grey_img);
}
