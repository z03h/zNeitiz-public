#include <stdio.h>
#include <stdlib.h>

#include "dust.h"
#include "salt.h"

void update_dust(Dust* dusts, unsigned int dust_num, int max_col, int min_col, unsigned int shape[]) {
    int col = dusts[dust_num].col;
    if (dusts[dust_num].active > 0){
        // active, randomly move direction
        //int col_rand = (int)(shape[1]/50), col_offset = 1;
        double row_offset = shape[0]/40;
        int row_rand = (int)(row_offset*2+1);
        // check for 0 div errors
        if (row_rand == 0) {
            row_rand = 3;
            row_offset = 1;
        }
        /*if (col_rand == 0) {
            col_rand = shape[1]/20;
        }*/
        dusts[dust_num].row += (double)(rand() % row_rand) - row_offset;
        dusts[dust_num].col += dusts[dust_num].x_velocity;
        if (dusts[dust_num].x_velocity < 13.0) {
            dusts[dust_num].x_velocity += 0.08;
        }

    } else if (col > max_col) {
        // force active
        dusts[dust_num].active++;
        dusts[dust_num].x_velocity = (double)(rand() % 200)/400 + 1;
    } else if (col > min_col) {
        int chance = rand() % (max_col - min_col);
        if (chance < (col - min_col)) {
            dusts[dust_num].active++;
            dusts[dust_num].x_velocity = (double)(rand() % 200)/400 + 1;
        }
    }
}

void draw_dust(Dust* dusts, unsigned int dust_num, unsigned char* arr, unsigned int offset, unsigned int shape[], unsigned int stride[]) {
    int row = (int)dusts[dust_num].row, col = (int)dusts[dust_num].col;
    if (in_array(row, col, shape) == 1) {
        // draw
        unsigned int index = rowcol_to_index(row, col, stride, offset);
        arr[index] = dusts[dust_num].R;
        arr[index+1] = dusts[dust_num].G;
        arr[index+2] = dusts[dust_num].B;
        arr[index+3] = dusts[dust_num].A;
    }
}

char in_array(unsigned int row, unsigned int col, unsigned int shape[]) {
    return ((row >= 0) && (row < shape[0]) && (col >= 0) && (col < shape[1])) ? 1:0;
}
