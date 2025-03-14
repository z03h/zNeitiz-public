struct rgb{
    double r,g,b;
};
typedef struct rgb RGB;

struct hsv{
    double h,s,v;
};
typedef struct hsv HSV;

struct replaced_color{
    double or,og,ob;
    int r,g,b;
};
typedef struct replaced_color Replaced;

struct RGB_to_LAB {
    double L, a, b;
};
typedef struct RGB_to_LAB LAB;

struct replaced_colors{
    Replaced *colors;
    int current, size;
};
typedef struct replaced_colors ReplacedColors;

struct to_replace{
    int* current;
    int size;
    char* colors;
};
typedef struct to_replace ToReplace;

LAB get_RGB_to_LAB(double, double, double);
double color_distance(double, double, double, double, double, double);
double color_distance2000(double, double, double, double, double, double);
RGB offset_rgb2(double, double, double, double, double, double, double, double, double);
double deg2Rad(double);
double rad2Deg(double);
void* random_colors(double[], int, double, char[], Replaced *, int* , int*);
ReplacedColors replace_colors(double[], int, double, unsigned char[], ReplacedColors, ToReplace);
void* create_ptr(int, int);
void free_ptr(void *);
