#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "salt.h"

void init_srand() {
    srand(time(NULL));
}

void range_sample(unsigned int *output, unsigned int max, unsigned int k) {
    unsigned int i = 0, j;

    for (;i < k; i++) {
        output[i] = i;
    }
    for (; i < max; i++) {
        j = rand() % (i+1);
        if (j < k) {
          output[j] = i;
        }
    }
}

unsigned char get_color(unsigned int type) {
    if (type == 1) {
        // pepper
        unsigned char colors[] = {20, 40, 80, 100};
        unsigned int weights[] = {2, 3, 3, 2};
        unsigned int sum_weights = 0;
        for (int i=0; i<4; i++) {
            sum_weights += weights[i];
        }
        unsigned int rnd = rand() % sum_weights;

        for (int i=0; i<4; i++) {
           if (rnd < weights[i]) {
                return colors[i];
           }
            rnd -= weights[i];
        }
    }else if (type == 0) {
        // salt
        unsigned char colors[] = {255, 220, 180, 140};
        unsigned int weights[] = {5, 3, 1, 1};
        unsigned int sum_weights = 0;
        for (int i=0; i<4; i++) {
            sum_weights += weights[i];
        }
        unsigned int rnd = rand() % sum_weights;

        for (int i=0; i<4; i++) {
            if (rnd < weights[i]) {
                return colors[i];
            }
            rnd -= weights[i];
        }
    } else if (type == 2){
        // water?
        return (unsigned char)255;
    } else {
        // mystery???
        return (unsigned char)127;
    }
    return (unsigned char)127;
}

unsigned int rowcol_to_index(unsigned int row, unsigned int col, unsigned int stride[], unsigned int offset) {
    return (row * stride[0]) + (col * stride[1]) + offset;
}

unsigned char is_filled(unsigned int row, unsigned int col, unsigned char* arr, unsigned int offset, unsigned int stride[]) {
    unsigned int index = rowcol_to_index(row, col, stride, offset);
    // increase by 3 to get A of RGBA
    index += 3;
    return (unsigned char)(arr[index]);
}

void draw_sand(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill) {
    unsigned int row = particles[particle_number].row, col = particles[particle_number].col;
    unsigned int index = rowcol_to_index(row, col, stride, offset);
    unsigned char color = (fill > 0) ? particles[particle_number].color : (unsigned char) 0;
    for (unsigned int current_index = index; current_index < index + 3; current_index++) {
        arr[current_index] = color;
    }
    arr[index+3] = (unsigned char)fill;
}

void draw_liquid(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill) {
    unsigned int row = particles[particle_number].row, col = particles[particle_number].col;
    unsigned int index = rowcol_to_index(row, col, stride, offset);
    unsigned char color = particles[particle_number].color;
    arr[index] = 0;
    arr[index+1] = (unsigned char)(color/3);
    arr[index+2] = (unsigned char)color;
    arr[index+3] = (unsigned char)fill;
}

void draw_piss(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill) {
    unsigned int row = particles[particle_number].row, col = particles[particle_number].col;
    unsigned int index = rowcol_to_index(row, col, stride, offset);
    arr[index] = (unsigned char) 222;
    arr[index+1] = (unsigned char)234;
    arr[index+2] = (unsigned char)20;
    arr[index+3] = (unsigned char)fill;
}

void update_sand(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int shape[], unsigned int stride[]) {
    unsigned int row = particles[particle_number].row, col = particles[particle_number].col;
    row++;
    if (row >= shape[0]) {
        // already hit the bottom of the image
        return;
    }

    // check spot right below
    if (is_filled(row, col, arr, 0, stride) < ALPHA_THRESHOLD) {
        // directly below is empty, move down
        draw_sand(particles, particle_number, arr, 0, stride, 0);
        particles[particle_number].row = row;
        particles[particle_number].col = col;
    }else {
        // below is filled, check left and right
        unsigned char left, right;
        right = (col < shape[1]-1) ? is_filled(row, col+1, arr, 0, stride) : 255;
        left = (col > 0 ) ? is_filled(row, col-1, arr, 0, stride) : 255;
        if (right >= ALPHA_THRESHOLD && left >= ALPHA_THRESHOLD) {
            // both filled
            // stay in current position
            return;
        }else if (right < ALPHA_THRESHOLD) {
            // right is open, move to the right
            draw_sand(particles, particle_number, arr, 0, stride, 0);
            particles[particle_number].row = row;
            particles[particle_number].col = col + 1;
        }else if (left < ALPHA_THRESHOLD) {
            // left is open, move to the left
            draw_sand(particles, particle_number, arr, 0, stride, 0);
            particles[particle_number].row = row;
            particles[particle_number].col = col - 1;
        }
    }
    draw_sand(particles, particle_number, arr, 0, stride, 255);
}

void update_liquid(Particle* particles, unsigned int particle_number, unsigned char* arr, unsigned int shape[], unsigned int stride[]) {
    unsigned int row = particles[particle_number].row, col = particles[particle_number].col;
    if ((row+1) >= shape[0]) {
        // already hit the bottom of the image
        return;
    }
    row++;
    // check spot right below
    if (is_filled(row, col, arr, 0, stride) < ALPHA_THRESHOLD) {
        // directly below is empty, move down
        draw_liquid(particles, particle_number, arr, 0, stride, 0);
        particles[particle_number].row = row;
        particles[particle_number].col = col;
    }else {
        // below is filled, check left and right
        unsigned char left, right;
        unsigned char space_checker;

        // check down right/down left 4? pixels
        for (space_checker=1; space_checker < 5; space_checker++){
            right = (col < (shape[1]-space_checker)) ? is_filled(row, col+space_checker, arr, 0, stride) : 255;
            left = (col >= space_checker) ? is_filled(row, col-space_checker, arr, 0, stride) : 255;
            if (right < ALPHA_THRESHOLD) {
                // right is open, move to the right
                draw_liquid(particles, particle_number, arr, 0, stride, 0);
                particles[particle_number].row = row;
                particles[particle_number].col = col + space_checker;
                draw_liquid(particles, particle_number, arr, 0, stride, 255);
                return;
            }else if (left < ALPHA_THRESHOLD) {
                // left is open, move to the left
                draw_liquid(particles, particle_number, arr, 0, stride, 0);
                particles[particle_number].row = row;
                particles[particle_number].col = col - space_checker;
                draw_liquid(particles, particle_number, arr, 0, stride, 255);
                return;
            }
        }

        // check directly right/left
        row--;
        // check right/left 3? pixels
        for (space_checker=1; space_checker < 4; space_checker++){
            right = (col < (shape[1]-space_checker)) ? is_filled(row, col+space_checker, arr, 0, stride) : 255;
            left = (col >= space_checker) ? is_filled(row, col-space_checker, arr, 0, stride) : 255;
            if (right < ALPHA_THRESHOLD) {
                // right is open, move to the right
                draw_liquid(particles, particle_number, arr, 0, stride, 0);
                particles[particle_number].row = row;
                particles[particle_number].col = col + space_checker;
                draw_liquid(particles, particle_number, arr, 0, stride, 255);
                return;
            }else if (left < ALPHA_THRESHOLD) {
                // left is open, move to the left
                draw_liquid(particles, particle_number, arr, 0, stride, 0);
                particles[particle_number].row = row;
                particles[particle_number].col = col - space_checker;
                draw_liquid(particles, particle_number, arr, 0, stride, 255);
                return;
            }
        }

    }

    draw_liquid(particles, particle_number, arr, 0, stride, 255);
}
