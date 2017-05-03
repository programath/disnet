//
// Created by sensetime on 2017/5/2.
//

#include "util.h"
#include "stdio.h"
#include <string.h>
#include <memory.h>
#define NUM_CHN 4
#define NUM_ROW 11
#define NUM_COL 11
#define OUT_CHN 8
#define WIN_ROW 3
#define WIN_COL 3

// for ()
// filter[i][j] = 1/(2*PI*sigma)*exp(-(i - centerx)*(i - centerx) - (j - centery) * (j - centery));

typedef float data_16f;
typedef ap_data_64<data_16f> data_64;
typedef ap_linebuffer<data_64, WIN_ROW, NUM_COL> LINEBUFFER;
typedef ap_window<data_64, WIN_ROW, WIN_COL> WINDOW;

#define NUM_CHN_DIV_4 (NUM_CHN >> 2)
#define OUT_CHN_DIV_4 (OUT_CHN >> 2)

void init_layers_64(data_64* input,data_64* filter, short in_chn, short in_h, short in_w, short out_chn, short k_h, short k_w){
    for (int i = 0; i < in_chn; i++)
        for (int j = 0; j < in_h; j++)
            for (int k = 0; k < in_w; k++)
                for (int l = 0; l < 4; l++)
                    input[i * in_h * in_w + j * in_w + k].a[l] = 1;


    for (int i = 0; i < out_chn; i++)
        for (int j = 0; j < k_h; j++)
            for (int k = 0; k < k_w; k++)
                for (int l = 0; l < 4; l++)
                    filter[i * k_h * k_w + j * k_w + k].a[l] = 1;
}
template <short k_h, short k_w>
data_64 conv2d_operator_64(ap_window<data_64, k_h, k_w>* input_window, ap_window<data_64, k_h, k_w>* filter){
    data_64 out_val;
    FILTER_CHN_LOOP:
    for (int fc = 0; fc < 4; fc++)
        INPUT_CHN_LOOP:
        for (int ic = 0; ic < 4; ic++)
            FILTER_ROW_LOOP:
            for (int i = 0; i < k_h; i++)
                FILTER_COL_LOOP:
                for (int j = 0; j < k_w; j++)
                    out_val.a[fc] = out_val.a[fc] + input_window->getval(i,j).a[ic] * filter->getval(i,j).a[fc];

    return out_val;
}

/*
conv2d:
input: c*h*w
filter: fc*fh*fw
stride: s
padding: pad
output: fc*((h-fh+2*padding)/s+1)*((w-fw+2*padding)/s+1)


*/

template <short in_h, short in_w, short k_h, short k_w>
void conv2d(data_64* input, data_64* filter, data_64* output,
          short in_chn,
          short out_chn,
          short stride, short padding) {
    short out_h = (in_h + 2 * padding - k_h)/stride + 1;
    short out_w = (in_w + 2 * padding - k_w)/stride + 1;

    FILTER_CHN_LOOP:
    for (int f = 0; f < out_chn; f++){
        ap_window<data_64, k_h, k_w> filter_buffer;
        for (int row = 0; row < k_h; row++)
            for (int col = 0; col < k_w; col++)
                filter_buffer.insert(filter[f * k_h * k_w + row * k_w + col], row, col);

        for (int k = 0; k < in_chn; k++){
            ap_linebuffer<data_64, k_h, in_w> line_buffer;
            for (int i = 0; i < in_h; i++) {
                ap_window<data_64, k_h, k_w> window_buffer;
                for (int j = 0; j < in_w; j++) {
                    line_buffer.shift_up(j);
                    data_64 temp2 = input[k * in_h * in_w + i * in_w + j];
                    line_buffer.insert_bottom(temp2, j);
                    window_buffer.shift_right();
                    for (int rindex = 0; rindex < k_h; rindex++) {
                        window_buffer.insert(line_buffer.getval(rindex, j), rindex, k_w - 1);
                    }
                    if (i >= k_h - 1 && j >= k_w - 1) {
                        short p = f * (in_h - k_h + 1) * (in_w - k_w + 1) + (i - k_h + 1) * (in_w - k_w + 1) + (j - k_w + 1);
                        output[p] += conv2d_operator_64<k_h, k_w>(&window_buffer, &filter_buffer);
                    }
                }
            }
        }
    }
}
/*
deconv2d:
input: c*h*w
filter: fc*fh*fw
stride: s
padding: pad
output: fc*((h-1)*s+1+2*pad)*((w-1)*s+1+2*pad)

*/
template <short in_h, short in_w, short k_h, short k_w>
void deconv2d(data_64* input, data_64* filter, data_64* output,
              short in_chn,
              short out_chn,
              short stride, short padding) {

    short out_h = (in_h - 1) * stride + 2 + 2 * padding - k_h;
    short out_w = (in_w - 1) * stride + 2 + 2 * padding - k_w;

    for (int f = 0; f < out_chn; f++){
        ap_window<data_64, k_h, k_w> filter_buffer;
        for (int row = 0; row < k_h; row++)
            for (int col = 0; col < k_w; col++)
                filter_buffer.insert(filter[f * in_h * in_w  + row * in_w + col], row, col);

        for (int k = 0; k < in_chn; k++){
            short right_shift = 2, up_shift = 2, out_pos_w = 0, out_pos_h = 0;
            ap_linebuffer<data_64, k_h, in_w> line_buffer;
            for (int i = 0; i < in_h + padding;){
                ap_window<data_64, k_h, k_w> window_buffer;

                out_pos_h++; up_shift++;
                out_pos_w = 0; right_shift = 2;
                for (int j = 0; j < in_w + padding;){
                    out_pos_w++; right_shift++;
                    window_buffer.shift_right();
                    if (right_shift == stride && j < in_w) {
                        right_shift = 0;
                        line_buffer.shift_up(j);
                        data_64 temp;
                        if (up_shift == stride && i < in_h){
                            temp = input[k * in_h * in_w + i * in_w + j];
                        }
                        line_buffer.insert_bottom(temp, j);

                        for (int rindex = 0; rindex < k_h; rindex++)
                            window_buffer.insert(line_buffer.getval(rindex, j), rindex, k_w - 1);
                        j = j + 1;
                    }
                    else {
                        data_64 temp;
                        if (j >= in_w) j = j + 1;
                        for (int rindex = 0; rindex < k_h; rindex++)
                            window_buffer.insert(temp, rindex, k_w - 1);

                    }
                    if (out_pos_h + padding >= k_h && out_pos_w + padding >= k_w) {
                        int p = f * out_h * out_w + (out_pos_h - k_h + padding) * out_w + out_pos_w - k_w + padding;
                       // printf("%d, %d, %d\n", p, i, up_shift);
                        output[p] += conv2d_operator_64<k_h, k_w>(&window_buffer, &filter_buffer);
                    }
                }
                if (up_shift == stride && i < in_h){
                    up_shift = 0; i++;
                }
                else if (i >= in_h) i = i + 1;
            }
        }
    }
}

data_64& relu(data_64& input){
    for (int i = 0; i < 4; i++)
        input.a[i] = input.a[i] > 0 ? input.a[i] : 0;
    return input;
}


void relu_layer(data_64* input, short in_chn, short in_h, short in_w){
    for (int i = 0; i < in_chn; i++)
        for (int j = 0; j < in_h; j++)
            for (int k = 0; k < in_w; k++)
                input[i * in_h * in_w + j * in_w + k] = relu(input[i * in_h * in_w + j * in_w + k]);
}


int main(){
    short in_ch = 1, in_h = 7, in_w = 7,
          out_ch = 2, k_h = 7, k_w = 7,
          stride = 3, padding = 3;

    data_64 image[1 * 7 * 7];
    data_64 filter[1 * 7 * 7];
    data_64 output[1 * 19 * 19];
                /* in_chn, in_h, in_w,
                 out_chn, k_h,  k_w,
                 stride, padding
                */
    init_layers_64(image, filter, 1, 7, 7, 1, 7, 7);
    deconv2d<7,7,7,7>(image, filter, output, 1, 1, 3, 3);
    for (int i = 0; i < 1 * 19 * 19; i++){
        if (i % 19 == 0)
            printf("\n");
        printf("%.2f ", output[i].a[2]);
    }


    return(0);
}


