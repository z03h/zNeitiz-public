#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "salt.h"
#include "debris.h"
#include "dust.h"

void c_particles(unsigned char* reference,
                 unsigned int shape[],
                 unsigned int stride[],
                 unsigned char* ret,
                 unsigned int frames,
                 unsigned int new_particle_count,
                 unsigned int skip,
                 unsigned int type){

    unsigned int MAX_INDEX = shape[0] * shape[1] * shape[2];

    Particle* particles = malloc(sizeof(Particle) * frames * new_particle_count * skip * 2);

    unsigned int current_offset, particle_counter;
    unsigned int total_particles = 0, skip_counter = 0;

    unsigned int *start_cols = malloc(sizeof(int) * new_particle_count);

    void (*update_particle)(Particle*, unsigned int, unsigned char*, unsigned int[], unsigned int[]);
    void (*draw_particle)(Particle*, unsigned int, unsigned char*, unsigned int, unsigned int[], unsigned char);

    switch(type){
        case 0:
        case 1:
            update_particle = &update_sand;
            draw_particle = &draw_sand;
            break;
        case 2:
            update_particle = &update_liquid;
            draw_particle = &draw_liquid;
            break;
        case 3:
            update_particle = &update_liquid;
            draw_particle = &draw_piss;
            break;
    }


    for (unsigned int frame=1; frame < frames; frame++) {
        // create each frame
        current_offset = MAX_INDEX * frame;

        // update by number of skips before drawing the final frame
        for (skip_counter=0; skip_counter < skip * 2; skip_counter++) {
            // random sample cols without replacement
            range_sample(start_cols, (unsigned int)60, new_particle_count);

            // add new particles
            for (particle_counter=0; particle_counter < new_particle_count; particle_counter++) {
                // check if spot is filled, if empty, add new salt
                if (is_filled(0, start_cols[particle_counter]+35, reference, 0, stride) == 0){
                    // get randow column and color
                    particles[total_particles].row = 0;
                    particles[total_particles].col = start_cols[particle_counter] + 35;
                    particles[total_particles].color = get_color(type);
                    total_particles++;
                }
            }

            // update each particle and reference image
            for (particle_counter=0; particle_counter < total_particles; particle_counter++) {
                (*update_particle)(particles, particle_counter, reference, shape, stride);
            }
        }

        // draw on the return array
        for (particle_counter=0; particle_counter < total_particles; particle_counter++) {
            draw_particle(particles, particle_counter, ret, current_offset, stride, (unsigned char)255);
        }
    }
    free(particles);
    free(start_cols);
}


void c_debris(int *active,
              unsigned char *reference,
              unsigned char *active_ref,
              unsigned int shape[],
              unsigned int stride[],
              unsigned char* ret,
              unsigned int frames,
              unsigned int percent){

    unsigned int MAX_INDEX = shape[0] * shape[1] * shape[2];

    Debris* debris_arr = malloc(sizeof(Debris) * shape[0] * shape[1]);

    unsigned int current_offset, debris_counter;
    unsigned int total_debris = 0, i;
    unsigned char A;

    // create debris
    for (unsigned int row=0; row < shape[0]; row++) {
        for (unsigned int col=0; col < shape[1]; col++){
            i = (row * stride[0]) + (col * stride[1]);
            A = reference[i+3];
            if (A > ALPHA_THRESHOLD) {
                // check if we should add
                if ((unsigned int)(rand() % 100) < percent) {
                    debris_arr[total_debris].R = reference[i];
                    debris_arr[total_debris].G = reference[i+1];
                    debris_arr[total_debris].B = reference[i+2];
                    debris_arr[total_debris].A = A;
                    debris_arr[total_debris].active = 1;
                    debris_arr[total_debris].row = (double)row;
                    debris_arr[total_debris].col = (double)col;
                    debris_arr[total_debris].row_velocity = (double)-((rand() % 15000)/1000 + 10);
                    debris_arr[total_debris].col_velocity = (double)(rand() % 40000)/1000 - 20;
                    total_debris++;
                } else {
                    // turn to transparent/remove
                    reference[i+3] = 0;
                }
            } else {
                reference[i+3] = 0;
            }
        }
    }

    // draw initial frame
    for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
        draw_debris(debris_arr, debris_counter, ret, 0, stride, 255);
    }

    for (unsigned int fc=1; fc < frames; fc++) {
        //printf("%u\n", fc);
        current_offset = fc * MAX_INDEX;
        //update inactive debris
        for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
            if (debris_arr[debris_counter].active == 0){
                update_debris(debris_arr, total_debris, debris_counter, active_ref, shape, stride, active, fc);
                update_debris(debris_arr, total_debris, debris_counter, active_ref, shape, stride, active, fc);
            }
        }
        // update all active debris
        for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
            if (debris_arr[debris_counter].active != 0){
                update_debris(debris_arr, total_debris, debris_counter, active_ref, shape, stride, active, fc);
            }
        }
        // draw all debris
        for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
            draw_debris(debris_arr, debris_counter, ret, current_offset, stride, 255);
        }
    }

    free(debris_arr);
}

void c_dust(unsigned char* reference,
            unsigned int shape[],
            unsigned int stride[],
            unsigned int max_dust,
            unsigned int frames,
            unsigned char* ret) {
    unsigned int max_col = (shape[1] * 1.2), min_col = max_col - shape[1]/3;

    unsigned int dust_counter, total_dust=0, i, current_offset;
    Dust * dusts = malloc(sizeof(Dust) * max_dust);

    unsigned int MAX_INDEX = shape[0] * shape[1] * shape[2];

    // create dust
    for (unsigned int row=0; row < shape[0]; row++) {
        for (unsigned int col=0; col<shape[1]; col++) {
            i = (row * stride[0]) + (col * stride[1]);
            unsigned char A = reference[i+3];
            if (A > ALPHA_THRESHOLD) {
                dusts[total_dust].R = reference[i];
                dusts[total_dust].G = reference[i+1];
                dusts[total_dust].B = reference[i+2];
                dusts[total_dust].A = A;
                dusts[total_dust].x_velocity = 0;
                dusts[total_dust].active = 0;
                dusts[total_dust].row = (double)row;
                dusts[total_dust].col = (double)col;
                total_dust++;
            } else {
                reference[i+3] = 0;
            }
        }
    }
    // draw initial frame
    for (dust_counter=0; dust_counter < total_dust; dust_counter++) {
        draw_dust(dusts, dust_counter, ret, 0, shape, stride);
    }
    // draw rest of the frames
    for (unsigned int fc=1; fc < frames; fc++) {
        current_offset = fc * MAX_INDEX;
        for (dust_counter=0; dust_counter < total_dust; dust_counter++) {
            update_dust(dusts, dust_counter, max_col, min_col, shape);
        }
        // draw all debris
        for (dust_counter=0; dust_counter < total_dust; dust_counter++) {
            draw_dust(dusts, dust_counter, ret, current_offset, shape, stride);
        }
        max_col -= 2;
        min_col -= 2;
    }
    free(dusts);
}


void c_crumble(int* active,
               unsigned char *active_ref,
               unsigned char *reference,
               unsigned int shape[],
               unsigned int stride[],
               unsigned char* ret,
               unsigned int frames){

    unsigned int MAX_INDEX = shape[0] * shape[1] * shape[2];

    Debris* debris_arr = malloc(sizeof(Debris) * shape[0] * shape[1]);

    unsigned int current_offset, debris_counter;
    unsigned int total_debris = 0, i;
    unsigned char A;

    // create debris
    for (unsigned int row=shape[0]-1; row < shape[0]; row--) {
        for (unsigned int col=0; col < shape[1]/2; col++) {
            i = (row * stride[0]) + (col * stride[1]);
            A = reference[i+3];
            if (A > ALPHA_THRESHOLD) {
                debris_arr[total_debris].R = reference[i];
                debris_arr[total_debris].G = reference[i+1];
                debris_arr[total_debris].B = reference[i+2];
                debris_arr[total_debris].A = A;
                debris_arr[total_debris].active = 0;
                debris_arr[total_debris].row = (double)row;
                debris_arr[total_debris].col = (double)col;
                debris_arr[total_debris].row_velocity = 0;
                debris_arr[total_debris].col_velocity = 0;
                total_debris++;
            } else {
                reference[i+3] = 0;
            }
            i = (row * stride[0]) + ((shape[1]-col-1) * stride[1]);
            A = reference[i+3];
            if (A > ALPHA_THRESHOLD) {
                debris_arr[total_debris].R = reference[i];
                debris_arr[total_debris].G = reference[i+1];
                debris_arr[total_debris].B = reference[i+2];
                debris_arr[total_debris].A = A;
                debris_arr[total_debris].active = 0;
                debris_arr[total_debris].row = (double)row;
                debris_arr[total_debris].col = (double)(shape[1]-col-1);
                debris_arr[total_debris].row_velocity = 0;
                debris_arr[total_debris].col_velocity = 0;
                total_debris++;
            } else {
                reference[i+3] = 0;
            }
        }
        if ((shape[1] % 2) == 1) {
            // odd
            i = (row * stride[0]) + ((shape[1]/2) * stride[1]);
            A = reference[i+3];
            if (A > ALPHA_THRESHOLD) {
                debris_arr[total_debris].R = reference[i];
                debris_arr[total_debris].G = reference[i+1];
                debris_arr[total_debris].B = reference[i+2];
                debris_arr[total_debris].A = A;
                debris_arr[total_debris].active = 0;
                debris_arr[total_debris].row = (double)row;
                debris_arr[total_debris].col = (double)(shape[1]/2);
                debris_arr[total_debris].row_velocity = 0;
                debris_arr[total_debris].col_velocity = 0;
                total_debris++;
            } else {
                reference[i+3] = 0;
            }
        }
    }

    // draw initial frame
    for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
        draw_debris(debris_arr, debris_counter, ret, 0, stride, 255);
    }

    for (unsigned int fc=1; fc < frames; fc++) {
        //printf("%u\n", fc);
        current_offset = fc * MAX_INDEX;
        for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
            update_debris(debris_arr, total_debris, debris_counter, active_ref, shape, stride, active, fc);
            update_debris(debris_arr, total_debris, debris_counter, active_ref, shape, stride, active, fc);
        }
        // draw all debris
        for (debris_counter=0; debris_counter < total_debris; debris_counter++) {
            draw_debris(debris_arr, debris_counter, ret, current_offset, stride, 255);
        }
    }

    free(debris_arr);
}
