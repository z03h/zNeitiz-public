#ifndef HEADER_DEBRIS
#define HEADER_DEBRIS

struct debris{
    unsigned char R, G, B, A;
    double row, col, row_velocity, col_velocity;
    char active;
};
typedef struct debris Debris;

void update_debris(Debris*, unsigned int, unsigned int, unsigned char*, unsigned int[], unsigned int[], int*, unsigned int);
void draw_debris(Debris*, unsigned int, unsigned char*, unsigned int, unsigned int[], unsigned char);
int is_active(int*, unsigned int, unsigned int, unsigned int, unsigned int[]);

#endif
