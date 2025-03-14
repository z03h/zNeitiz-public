#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "color_replace.h"

#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define DZERO       0.00001 // zero for double compare
#define NDZERO      -0.00001 // zero for double compare

//Color math mumbo jumbo
LAB get_RGB_to_LAB(double var_R, double var_G, double var_B){
    LAB c;
    // RGB to XYZ
    var_R /= 255.0 ;
    var_G /= 255.0 ;
    var_B /= 255.0 ;

    if ( var_R > 0.04045 ){
        var_R = pow(( ( var_R + 0.055 ) / 1.055 ), 2.4);
    }else{
       var_R /= 12.92;
    }

    if ( var_G > 0.04045 ){
        var_G = pow(( ( var_G + 0.055 ) / 1.055 ), 2.4);
    }else{
        var_G /= 12.92;
    }

    if ( var_B > 0.04045 ){
        var_B = pow(( ( var_B + 0.055 ) / 1.055 ), 2.4);
    }else{
        var_B /= 12.92;
    }

    var_R *= 100.0;
    var_G *= 100.0;
    var_B *= 100.0;

    // RGB to XYZ values
    double X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
    double Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
    double Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

    double refX = 95.0489;
    double refY = 100.0;
    double refZ = 108.8840;

    // XYZ to LAB
    X /= refX;
    Y /= refY;
    Z /= refZ;

    if ( X > 0.008856 ){
        X = cbrt(X);
    }else{
        X = ( 7.787 * X ) + (16.0 / 116.0);
    }

    if ( Y > 0.008856 ){
        Y = cbrt(Y);
    }else{
        Y = ( 7.787 * Y ) + ( 16.0 / 116.0 );
    }
    if ( Z > 0.008856 ){
        Z = cbrt(Z);
    }else{
        Z = ( 7.787 * Z ) + (double)( 16.0 / 116.0 );
    }

    // XYZ to LAB values in struct
    c.L = ( 116.0 * Y ) - 16.0;
    c.a = 500.0 * ( X - Y );
    c.b = 200.0 * ( Y - Z );
    //printf("%f %f %f\n",c.L, c.a, c.b );
    return c;
}

double deg2Rad(double deg) {
    return (deg * (M_PI / 180.0));
}

double rad2Deg(double rad) {
    return ((180.0 / M_PI) * rad);
}

double color_distance(double r1, double g1, double b1, double r2, double g2, double b2){
    LAB lab1 = get_RGB_to_LAB(r1, g1, b1);
    LAB lab2 = get_RGB_to_LAB(r2, g2, b2);
    double l = pow((lab1.L - lab2.L), 2.0);
    double a = pow((lab1.a - lab2.a), 2.0);
    double b = pow((lab1.b - lab2.b), 2.0);
    double result = sqrt((l+a+b));
    //printf("lab = %f %f %f == result %f\n", l,a,b,result);
    return result;
}

double color_distance2000(double r1, double g1, double b1, double r2, double g2, double b2){
    // https://github.com/gfiumara/CIEDE2000

    LAB lab1 = get_RGB_to_LAB(r1, g1, b1);
    LAB lab2 = get_RGB_to_LAB(r2, g2, b2);
    //printf("lab1 %f %f  %f\n", lab1.L, lab1.a, lab1.b);
    //printf("lab2 %f %f  %f\n", lab2.L, lab2.a, lab2.b);
    double k_L = 1.0, k_C = 1.0, k_H = 1.0;
    double deg360InRad = deg2Rad(360.0);
    double deg180InRad = deg2Rad(180.0);
    double pow25To7 = 6103515625.0; /* pow(25, 7) */

    /*
     * Step 1
     */
    /* Equation 2 */
    double C1 = sqrt((lab1.a * lab1.a) + (lab1.b * lab1.b));
    double C2 = sqrt((lab2.a * lab2.a) + (lab2.b * lab2.b));
    /* Equation 3 */
    double barC = (C1 + C2) / 2.0;
    /* Equation 4 */
    double G = 0.5 * (1 - sqrt(pow(barC, 7) / (pow(barC, 7) + pow25To7)));
    /* Equation 5 */
    double a1Prime = (1.0 + G) * lab1.a;
    double a2Prime = (1.0 + G) * lab2.a;
    /* Equation 6 */
    double CPrime1 = sqrt((a1Prime * a1Prime) + (lab1.b * lab1.b));
    double CPrime2 = sqrt((a2Prime * a2Prime) + (lab2.b * lab2.b));
    /* Equation 7 */
    double hPrime1;
    if (lab1.b == 0 && a1Prime == 0){
        hPrime1 = 0.0;
    }else {
        hPrime1 = atan2(lab1.b, a1Prime);
        /*
         * This must be converted to a hue angle in degrees between 0
         * and 360 by addition of 2􏰏 to negative hue angles.
         */
        if (hPrime1 < 0)
            hPrime1 += deg360InRad;
    }
    double hPrime2;
    if (lab2.b == 0 && a2Prime == 0)
        hPrime2 = 0.0;
    else {
        hPrime2 = atan2(lab2.b, a2Prime);
        /*
         * This must be converted to a hue angle in degrees between 0
         * and 360 by addition of 2􏰏 to negative hue angles.
         */
        if (hPrime2 < 0)
            hPrime2 += deg360InRad;
    }

    /*
     * Step 2
     */
    /* Equation 8 */
    double deltaLPrime = lab2.L - lab1.L;
    /* Equation 9 */
    double deltaCPrime = CPrime2 - CPrime1;
    /* Equation 10 */
    double deltahPrime;
    double CPrimeProduct = CPrime1 * CPrime2;
    if (CPrimeProduct == 0)
        deltahPrime = 0;
    else {
        /* Avoid the fabs() call */
        deltahPrime = hPrime2 - hPrime1;
        if (deltahPrime < -deg180InRad)
            deltahPrime += deg360InRad;
        else if (deltahPrime > deg180InRad)
            deltahPrime -= deg360InRad;
    }
    /* Equation 11 */
    double deltaHPrime = 2.0 * sqrt(CPrimeProduct) *
        sin(deltahPrime / 2.0);

    /*
     * Step 3
     */
    /* Equation 12 */
    double barLPrime = (lab1.L + lab2.L) / 2.0;
    /* Equation 13 */
    double barCPrime = (CPrime1 + CPrime2) / 2.0;
    /* Equation 14 */
    double barhPrime, hPrimeSum = hPrime1 + hPrime2;
    if (CPrime1 * CPrime2 == 0 && CPrime1 * CPrime2 == 0) {
        barhPrime = hPrimeSum;
    } else {
        if (fabs(hPrime1 - hPrime2) <= deg180InRad)
            barhPrime = hPrimeSum / 2.0;
        else {
            if (hPrimeSum < deg360InRad)
                barhPrime = (hPrimeSum + deg360InRad) / 2.0;
            else
                barhPrime = (hPrimeSum - deg360InRad) / 2.0;
        }
    }
    /* Equation 15 */
    double T = 1.0 - (0.17 * cos(barhPrime - deg2Rad(30.0))) +
        (0.24 * cos(2.0 * barhPrime)) +
        (0.32 * cos((3.0 * barhPrime) + deg2Rad(6.0))) -
        (0.20 * cos((4.0 * barhPrime) - deg2Rad(63.0)));
    /* Equation 16 */
    double deltaTheta = deg2Rad(30.0) *
        exp(-pow((barhPrime - deg2Rad(275.0)) / deg2Rad(25.0), 2.0));
    /* Equation 17 */
    double R_C = 2.0 * sqrt(pow(barCPrime, 7.0) /
        (pow(barCPrime, 7.0) + pow25To7));
    /* Equation 18 */
    double S_L = 1 + ((0.015 * pow(barLPrime - 50.0, 2.0)) /
        sqrt(20 + pow(barLPrime - 50.0, 2.0)));
    /* Equation 19 */
    double S_C = 1 + (0.045 * barCPrime);
    /* Equation 20 */
    double S_H = 1 + (0.015 * barCPrime * T);
    /* Equation 21 */
    double R_T = (-sin(2.0 * deltaTheta)) * R_C;

    /* Equation 22 */
    double deltaE = sqrt(
        pow(deltaLPrime / (k_L * S_L), 2.0) +
        pow(deltaCPrime / (k_C * S_C), 2.0) +
        pow(deltaHPrime / (k_H * S_H), 2.0) +
        (R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H))));

    return deltaE;
}

char get_rand(){
    return (unsigned char) (rand() % 255);
}
void set_rand(){
    srand(time(0));
}

// HSV conversion from https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
HSV rgb_to_hsv(double r, double g, double b){
    HSV out;
    double min, max, delta;
    r /= 255;
    g /= 255;
    b /= 255;

    min = r < g ? r : g;
    min = min < b ? min : b;

    max = r > g ? r : g;
    max = max > b ? max : b;

    out.v = max; // v
    delta = max - min;
    if (delta < DZERO)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = 0.0; // its now undefined
        return out;
    }
    if( r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( g - b ) / delta;        // between yellow & magenta
    else
    if( g >= max )
        out.h = 2.0 + ( b - r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( r - g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;

}

RGB hsv_to_rgb(double h, double s, double v){
    double hh, p, q, t, ff;
    long i;
    RGB out;

    if(s < 0.001) {       // < is bogus, just shuts up warnings
        out.r = 255 * v;
        out.g = 255 * v;
        out.b = 255 * v;
        return out;
    }
    hh = h;
    if(hh >= 360.0){
        hh = 0.0;
    }
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = v;
        break;
    case 5:
    default:
        out.r = v;
        out.g = p;
        out.b = q;
        break;
    }
    out.r *= 255;
    out.g *= 255;
    out.b *= 255;
    return out;

}

RGB offset_rgb2(
    double refr, double refg, double refb, // originals
    double offsetr, double offsetg, double offsetb, // to get offset from original
    double r, double g, double b //apply offset here
){
    HSV ref, offset, rgb;
    ref = rgb_to_hsv(refr, refg, refb);
    offset = rgb_to_hsv(offsetr, offsetg, offsetb);
    // pure grayscale override
    // if (offset.s < 1){}
    rgb = rgb_to_hsv(r, g, b);
    // use saturation or value to limit hue rotation
    double hue_offset, hue_ratio, hs_ratio, hv_ratio;
    hs_ratio = (ref.s + offset.s)/2;
    hv_ratio = (ref.v + offset.v)/2;

    hue_ratio = hs_ratio < hv_ratio ? hs_ratio:hv_ratio;
    /*
    printf("ref %.2f %.2f %.2f\n", ref.h, ref.s, ref.v);
    printf("off %.2f %.2f %.2f\n", offset.h, offset.s, offset.v);
    printf("rgb %.2f %.2f %.2f\n", rgb.h, rgb.s, rgb.v);
    */
    hue_offset = (offset.h - ref.h);
    if (hue_offset < 0 && hue_offset < -180){
        hue_offset += 360.0;
    }else if (hue_offset > 0 && hue_offset > 180){
        hue_offset -= 360;
    }
    rgb.h += hue_offset * hue_ratio;
    if (ref.s > 0.02){
        double s_diff, s_ratio;
        s_ratio = (rgb.s * offset.s/ref.s) - rgb.s;
        s_diff = offset.s - ref.s;
        rgb.s += (((s_ratio < 0) ? -s_ratio : s_ratio) > ((s_diff < 0) ? -s_diff : s_diff)) ? s_diff : s_ratio;
    } else {
        rgb.s += (offset.s - ref.s);
    }

    if (ref.v > 0.02){
        double v_diff, v_ratio;
        v_ratio = (rgb.v * offset.v/ref.v) - rgb.v;
        v_diff = offset.v - ref.v;
        rgb.v += (((v_ratio < 0) ? -v_ratio : v_ratio) > ((v_diff < 0) ? -v_diff : v_diff)) ? v_diff : v_ratio;
    } else {
        rgb.v += (offset.v - ref.v);
    }

    if (rgb.h < 0){
        rgb.h += 360;
    } else if (rgb.h >= 360){
        rgb.h -= 360;
    }
    if (rgb.s < 0){
        rgb.s = 0.0;
    } else if (rgb.s > 1.0){
        rgb.s = 1.0;
    }
    if (rgb.v < 0){
        rgb.v = 0.0;
    } else if (rgb.v > 1.0){
        rgb.v = 1.0;
    }
    RGB ret = hsv_to_rgb(rgb.h, rgb.s, rgb.v);
    /*
    printf("rgb hsv %.2f %.2f %.2f\n", rgb.h, rgb.s, rgb.v);
    printf("rgb rgb %.2f %.2f %.2f\n\n", x.r, x.g, x.b);
    */
    return ret;
}

void* random_colors(double input[], int len, double max_dist, char output[], Replaced *colors, int* current_index, int*current_size){
    //check if multiple of 3
    if (len % 3 != 0){
        return NULL;
    }
    Replaced var;
    void * temp;

    //printf("C %d %d %d, %d %d\n", colors[0].r, colors[0].g, colors[0].b, *current_index, *current_size);
    int min_index;
    double dist, min_dist, r,g,b;
    for (int i=0; i<len; i+=3){
        min_dist = 512;
        min_index = -1;
        // for each 3 rgb values
        r = input[i];
        g = input[i+1];
        b = input[i+2];
        if (r+g+b > 5.0){
            if (*current_index > 0){
                // if any replaced colors, check distance
                for (int j=0; j< *current_index; j++){
                    //loop over replaced colors
                    dist = color_distance2000(r,g,b, colors[j].or, colors[j].og, colors[j].ob);
                    if (dist < 0.1){
                        //close enough to the same color, replace
                        min_index = j;
                        break;
                    }else if (dist < min_dist && dist < max_dist){
                        min_dist = dist;
                        min_index = j;
                    }
                }
                if (min_index >= 0){
                    //found a matching color
                    output[i] = (char)colors[min_index].r;
                    output[i+1] = (char)colors[min_index].g;
                    output[i+2] = (char)colors[min_index].b;
                }else{
                    //no color found, add random color
                    output[i] = get_rand();
                    output[i+1] = get_rand();
                    output[i+2] = get_rand();
                    //add color to replaced colors
                    //set original color
                    colors[*current_index].or = r;
                    colors[*current_index].og = g;
                    colors[*current_index].ob = b;
                    //set replaced color
                    colors[*current_index].r = output[i];
                    colors[*current_index].g = output[i+1];
                    colors[*current_index].b = output[i+2];
                    //increment current index
                    (*current_index)++;
                    //check if need to realloc
                    if (*current_index >= *current_size){
                        *current_size += 20;
                        //printf("realloc1 size, %d  index %d\n", *current_size, *current_index);
                        temp = realloc(colors, sizeof(var)*(*current_size));
                        if (temp == NULL){
                            free(colors);
                            return NULL;
                        }
                        colors = temp;
                    }
                }
            }else{
                //add random color
                output[i] = get_rand();
                output[i+1] = get_rand();
                output[i+2] = get_rand();
                //add color to replaced colors
                //set original color
                colors[*current_index].or = r;
                colors[*current_index].og = g;
                colors[*current_index].ob = b;
                //set replaced color
                colors[*current_index].r = output[i];
                colors[*current_index].g = output[i+1];
                colors[*current_index].b = output[i+2];
                //increment current index
                (*current_index)++;
                //check if need to realloc
                if (*current_index >= *current_size){
                    *current_size += 20;
                    //printf("realloc2 size, %d  index %d\n", *current_size, *current_index);
                    temp = realloc(colors, sizeof(var)*(*current_size));
                    if (temp == NULL){
                        free(colors);
                        return NULL;
                    }
                    colors = temp;
                }
            }
        }else{
            //sum is < threshold, use original color
            output[i] = (char)r;
            output[i+1] = (char)g;
            output[i+2] = (char)b;
        }
    }
    return colors;
}
/*
unsigned char offset_rgb(unsigned char value, char offset){
    if (offset < 0){
        return (value > (unsigned char)(value-offset)) ? 255 : (value-offset);
    }else if (offset > 0){
        return (value > (unsigned char)(value-offset)) ? (value-offset) : 0;
    }else{
        return value;
    }
}*/
ReplacedColors replace_colors(double input[], int len, double max_dist, unsigned char output[], ReplacedColors all_colors, ToReplace other_colors){
    //check if multiple of 3
    if (len % 3 != 0){
        return all_colors;
    }
    RGB offset;
    Replaced var;
    void * temp;

    //printf("C , %d %d\n", all_colors.current, all_colors.size);
    int min_index;
    double dist, min_dist, r,g,b;
    for (int i=0; i<len; i+=3){
        min_dist = 512;
        min_index = -1;
        // for each 3 rgb values
        r = input[i];
        g = input[i+1];
        b = input[i+2];
        if (r+g+b > 0.0){
            if (all_colors.current > 0){
                // if any replaced colors, check distance
                for (int j=0; j< all_colors.current; j++){
                    //loop over replaced colors
                    dist = color_distance2000(
                        all_colors.colors[j].or, all_colors.colors[j].og, all_colors.colors[j].ob,
                        r,g,b
                    );
                    if (dist < 0.1){
                        //close enough to the same color, replace
                        min_index = j;
                        break;
                    }else if (dist < min_dist && dist < max_dist){
                        min_dist = dist;
                        min_index = j;
                    }
                }
                if (min_index >= 0){
                    //found a matching colors, calc for offset

                    offset = offset_rgb2(
                        all_colors.colors[min_index].or, all_colors.colors[min_index].og, all_colors.colors[min_index].ob,
                        r, g, b,
                        all_colors.colors[min_index].r, all_colors.colors[min_index].g, all_colors.colors[min_index].b
                    );
                    output[i] = (unsigned char)offset.r;
                    output[i+1] = (unsigned char)offset.g;
                    output[i+2] = (unsigned char)offset.b;
                    /*
                    output[i] = offset_rgb(all_colors.colors[min_index].r, r-all_colors.colors[min_index].or);
                    output[i+1] = offset_rgb(all_colors.colors[min_index].g, g-all_colors.colors[min_index].og);
                    output[i+2] = offset_rgb(all_colors.colors[min_index].b, b-all_colors.colors[min_index].ob);
                    */
                }else{
                    //no color found, get next color
                    if (*(other_colors.current) >= other_colors.size){
                        *other_colors.current = 0;
                    }
                    output[i] = (unsigned char) other_colors.colors[(*other_colors.current)++];
                    output[i+1] = (unsigned char) other_colors.colors[(*other_colors.current)++];
                    output[i+2] = (unsigned char) other_colors.colors[(*other_colors.current)++];


                    //add color to replaced colors
                    //set original color
                    all_colors.colors[all_colors.current].or = r;
                    all_colors.colors[all_colors.current].og = g;
                    all_colors.colors[all_colors.current].ob = b;
                    //set replaced color
                    all_colors.colors[all_colors.current].r = output[i];
                    all_colors.colors[all_colors.current].g = output[i+1];
                    all_colors.colors[all_colors.current].b = output[i+2];
                    //increment current index
                    all_colors.current++;
                    //check if need to realloc
                    if (all_colors.current >= all_colors.size){
                        all_colors.size += 20;
                        //printf("realloc1 size, %d  index %d\n", all_colors.size, all_colors.current);
                        temp = realloc(all_colors.colors, sizeof(var)*(all_colors.size));
                        if (temp == NULL){
                            free(all_colors.colors);
                            return all_colors;
                        }
                        all_colors.colors = temp;
                    }
                }
            }else{
                //no previous colors, get next color
                if (*(other_colors.current) >= other_colors.size){
                    *other_colors.current = 0;
                }
                output[i] = (char) other_colors.colors[(*other_colors.current)++];
                output[i+1] = (char) other_colors.colors[(*other_colors.current)++];
                output[i+2] = (char) other_colors.colors[(*other_colors.current)++];


                //add color to replaced colors
                //set original color
                all_colors.colors[all_colors.current].or = r;
                all_colors.colors[all_colors.current].og = g;
                all_colors.colors[all_colors.current].ob = b;
                //set replaced color
                all_colors.colors[all_colors.current].r = output[i];
                all_colors.colors[all_colors.current].g = output[i+1];
                all_colors.colors[all_colors.current].b = output[i+2];
                //increment current index
                all_colors.current++;
                //check if need to realloc
                if (all_colors.current >= all_colors.size){
                    all_colors.size += 20;
                    //printf("realloc2 size, %d  index %d\n", all_colors.size, all_colors.current);
                    temp = realloc(all_colors.colors, sizeof(var)*(all_colors.size));
                    if (temp == NULL){
                        free(all_colors.colors);
                        return all_colors;
                    }
                    all_colors.colors = temp;
                }
            }
        }else{
            //sum is < threshold, use original color

            output[i] = (char)r;
            output[i+1] = (char)g;
            output[i+2] = (char)b;
        }
    }
    return all_colors;
}

void free_ptr(void * ptr){
    free(ptr);
}

void* create_ptr(int itemsize, int size){
    void *ptr = malloc(itemsize * size);

    return ptr;
}
