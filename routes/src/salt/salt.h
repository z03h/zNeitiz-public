#ifndef HEADER_SALT
#define HEADER_SALT

struct particle{
    unsigned int row, col;
    unsigned char color;
};
typedef struct particle Particle;

# define ALPHA_THRESHOLD 100

void init_srand();
void range_sample(unsigned int *output, unsigned int max, unsigned int k);
unsigned char get_color(unsigned int type);
unsigned int rowcol_to_index(unsigned int row, unsigned int col, unsigned int stride[], unsigned int offset);
unsigned char is_filled(unsigned int row, unsigned int col, unsigned char* arr, unsigned int offset, unsigned int stride[]);
void draw_sand(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill);
void draw_liquid(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill);
void draw_piss(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill);
void update_sand(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int shape[], unsigned int stride[]);
void update_liquid(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int shape[], unsigned int stride[]);

#endif
