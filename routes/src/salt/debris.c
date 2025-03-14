#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "debris.h"
#include "salt.h"


void update_debris(Debris* debris_arr, unsigned int total_debris, unsigned int debris_num, unsigned char* arr, unsigned int shape[], unsigned int stride[], int* active_arr, unsigned int fc) {
    if (debris_arr[debris_num].active == 1){
        // active, apply velocity
        if (debris_arr[debris_num].row_velocity > 0){
            // moving down
            double row_step, col_step, current_row, current_col, col_stepped, row_stepped;
            unsigned int step, index, row_offset = shape[1];
            char side, below_side, below;
            current_row = debris_arr[debris_num].row;
            current_col = debris_arr[debris_num].col;
            step = (fabs(debris_arr[debris_num].row_velocity) > fabs(debris_arr[debris_num].col_velocity)) ? (unsigned int)fabs(debris_arr[debris_num].row_velocity) : (unsigned int)fabs(debris_arr[debris_num].col_velocity);
            if (step == 0){
                step = 1;
            }
            row_step = debris_arr[debris_num].row_velocity/step;
            col_step = debris_arr[debris_num].col_velocity/step;
            for (unsigned int i=0; i < step; i++){
                row_stepped = current_row + row_step;
                col_stepped = current_col + col_step;
                if (row_stepped < 0 || current_row < 0){
                    row_stepped = debris_arr[debris_num].row + debris_arr[debris_num].row_velocity;
                    col_stepped = debris_arr[debris_num].col + debris_arr[debris_num].col_velocity;
                    break;
                }
                if (row_stepped >= shape[0]){
                    //turn inactive
                    debris_arr[debris_num].active = 0;
                    debris_arr[debris_num].row = shape[0] - 1;
                    debris_arr[debris_num].col = current_col;
                    // set inactive at index
                    index = (unsigned int)((unsigned int)debris_arr[debris_num].row * row_offset + (unsigned int)debris_arr[debris_num].col);
                    active_arr[index]++;
                    break;
                }
                if (col_stepped < 0){
                    col_step = -col_step;
                    col_stepped = -col_stepped;
                    debris_arr[debris_num].col_velocity *= -0.96;
                } else if (col_stepped >= shape[1]-1) {
                    col_stepped = shape[1]* 2 - col_stepped - 2;
                    col_step = -col_step;
                    debris_arr[debris_num].col_velocity *= -0.96;
                }
                below = is_active(active_arr, (unsigned int)(row_stepped), (unsigned int)current_col, row_offset, shape);
                side = is_active(active_arr, (unsigned int)(current_row), (unsigned int)(col_stepped), row_offset, shape);
                below_side = is_active(active_arr, (unsigned int)(row_stepped), (unsigned int)(col_stepped), row_offset, shape);
                if (below_side > 0){
                    // hit the edge or hit an inactive pixel
                    // become inactive
                    debris_arr[debris_num].active = 0;
                    row_stepped -= row_step;
                    debris_arr[debris_num].row = row_stepped;
                    debris_arr[debris_num].col = col_stepped;
                    // set inactive at index
                    index = (unsigned int)((unsigned int)row_stepped * row_offset + (unsigned int)col_stepped);
                    active_arr[index]++;
                    break;
                }
                current_row = row_stepped;
                current_col = col_stepped;
            }

            // if still active
            if (debris_arr[debris_num].active != 0){
                debris_arr[debris_num].row = row_stepped;
                debris_arr[debris_num].col = col_stepped;
                if (debris_arr[debris_num].row_velocity < 20){
                    debris_arr[debris_num].row_velocity++;
                }
            }
        } else {
            // moving up, don't worry about hitting anything except col walls
            debris_arr[debris_num].row += debris_arr[debris_num].row_velocity;

            // move left/right
            debris_arr[debris_num].col += debris_arr[debris_num].col_velocity;

            // update row velocity
            debris_arr[debris_num].row_velocity++;

            // "bounce" off walls, reduce col velocity, going up so reduce row velocity a bit
            if (debris_arr[debris_num].col < 0){
                debris_arr[debris_num].col = -debris_arr[debris_num].col;
                debris_arr[debris_num].col_velocity = -debris_arr[debris_num].col_velocity * 0.9;
                debris_arr[debris_num].row_velocity = debris_arr[debris_num].row_velocity * 0.9;
            } else if (debris_arr[debris_num].col >= shape[1]) {
                debris_arr[debris_num].col = shape[1]* 2 - debris_arr[debris_num].col - 2;
                debris_arr[debris_num].col_velocity = -debris_arr[debris_num].col_velocity * 0.9;
                debris_arr[debris_num].row_velocity = debris_arr[debris_num].row_velocity * 0.9;
            } else {
                debris_arr[debris_num].col_velocity *= 0.9;
            }
        }
    } else {
        // not active, apply regular sand movement
        unsigned int row = (unsigned int)debris_arr[debris_num].row, col = (unsigned int)debris_arr[debris_num].col, index;
        row++;
        if (row >= shape[0]) {
            // already hit the bottom of the image
            return;
        }

        // check spot right below
        if (is_filled(row, col, arr, 0, stride) < ALPHA_THRESHOLD) {
            // directly below is empty, move down
            index = (unsigned int)((unsigned int)row * shape[1] + (unsigned int)col);
            active_arr[index]--;
            draw_debris(debris_arr, debris_num, arr, 0, stride, 0);
            debris_arr[debris_num].row = row;
            debris_arr[debris_num].col = col;
        }else {
            // below is filled, check left and right
            unsigned char left, right;
            right = (col < shape[1]-1) ? is_filled(row, col+1, arr, 0, stride) : 255;
            left = (col > 0 ) ? is_filled(row, col-1, arr, 0, stride) : 255;
            if ((right >= ALPHA_THRESHOLD)  && (left >= ALPHA_THRESHOLD)) {
                // both filled
                // stay in current position
                return;
            }else if (right < ALPHA_THRESHOLD) {
                // right is open, move to the right
                index = (unsigned int)((unsigned int)row * shape[1] + (unsigned int)col);
                active_arr[index]--;
                draw_debris(debris_arr, debris_num, arr, 0, stride, 0);
                debris_arr[debris_num].row = row;
                debris_arr[debris_num].col = col + 1;
            }else if (left < ALPHA_THRESHOLD) {
                // left is open, move to the left
                draw_debris(debris_arr, debris_num, arr, 0, stride, 0);
                index = (unsigned int)((unsigned int)row * shape[1] + (unsigned int)col);
                active_arr[index]--;
                debris_arr[debris_num].row = row;
                debris_arr[debris_num].col = col - 1;
            }
        }
        index = (unsigned int)((unsigned int)row * shape[1] + (unsigned int)col);
        active_arr[index]++;
        draw_debris(debris_arr, debris_num, arr, 0, stride, 255);
    }
}

void draw_debris(Debris* debris_arr, unsigned int debris_num, unsigned char* arr, unsigned int offset, unsigned int stride[], unsigned char fill) {
    int row = debris_arr[debris_num].row, col = debris_arr[debris_num].col;
    if (row < 0){
        return;
    }
    unsigned int index = rowcol_to_index((unsigned int)row, (unsigned int)col, stride, offset);
    arr[index] = debris_arr[debris_num].R;
    arr[index+1] = debris_arr[debris_num].G;
    arr[index+2] = debris_arr[debris_num].B;
    arr[index+3] = (fill != 0) ? debris_arr[debris_num].A : 0;
}

int is_active(int* active_arr, unsigned int row, unsigned int col, unsigned int row_offset, unsigned int shape[]) {
    unsigned int index = row * row_offset + col;
    if (row > shape[0]){
        return 1;
    }
    if (row < 0){
        return 0;
    }
    return (active_arr[index]);
}
