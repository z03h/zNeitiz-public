#ifndef HEADER_DUST
#define HEADER_DUST

struct dust{
    double row, col, x_velocity;
    unsigned char R, G, B, A;
    char active;
};
typedef struct dust Dust;

void update_dust(Dust*, unsigned int, int, int, unsigned int[]);
void draw_dust(Dust*, unsigned int, unsigned char*, unsigned int, unsigned int[], unsigned int[]);
char in_array(unsigned int, unsigned int, unsigned int[]);
#endif
