//
// Created by sensetime on 2017/5/2.
//
// Instruction:
// 1. we can create more than one instance of 

#include <ap_fixed.h>
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


typedef ap_data_64<data_16f> data_64;
typedef ap_linebuffer<data_64, WIN_ROW, NUM_COL> LINEBUFFER;
typedef ap_window<data_64, WIN_ROW, WIN_COL> WINDOW;

#define NUM_CHN_DIV_4 (NUM_CHN >> 2)
#define OUT_CHN_DIV_4 (OUT_CHN >> 2)

#pragma SDS data access_pattern(input:SEQUENTIAL, filter:SEQUENTIAL)
void init_layers_64(data_pack<data_16f> input[121],data_pack<data_16f> filter[9], short in_chn, short in_h, short in_w, short out_chn, short k_h, short k_w){
    int cnt = 0;
	for (int i = 0; i < 1; i++)
        for (int j = 0; j < 11; j++)
            for (int k = 0; k < 11; k++){
#pragma AP pipeline II=1
                input[cnt].a0 = 1;
                input[cnt].a1 = 2;
                input[cnt].a2 = 3;
                input[cnt].a3 = 4;
                cnt++;

            }

	cnt = 0;
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++){
#pragma AP pipeline
                filter[cnt].a0 = 1;
                filter[cnt].a1 = 2;
                filter[cnt].a2 = 3;
                filter[cnt].a3 = 4;
                cnt++;
            }
}
template <short k_h, short k_w>
data_64 conv2d_operator_64(ap_window<data_64, k_h, k_w>* input_window, ap_window<data_64, k_h, k_w>* filter){
    data_64 out_val;
    FILTER_CHN_LOOP:
    for (int fc = 0; fc < BITWIDTH; fc++)
        INPUT_CHN_LOOP:
        for (int ic = 0; ic < BITWIDTH; ic++)
            FILTER_ROW_LOOP:
            for (int i = 0; i < k_h; i++)
                FILTER_COL_LOOP:
                for (int j = 0; j < k_w; j++)
#pragma AP pipeline
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

template <short in_chn, short in_h, short in_w, short out_chn, short k_h, short k_w, short out_h, short out_w>
void conv2d(data_pack<data_16f> input[in_chn * in_h * in_w],data_64 filter[out_chn * k_h * k_w], data_64 output[out_chn * out_h * out_w],
          short stride, short padding) {
#pragma HLS INLINE
//#pragma HLS array_partition variable=input cyclic factor = 4 dim = 1
#pragma HLS array_partition variable=filter cyclic factor = 2 dim = 1
#pragma HLS array_partition variable=output cyclic factor = 4 dim = 1
	data_64 inputBuf[in_chn][in_h][in_w];
	//data_pack<data_16f> filterBuf[out_chn][k_h][k_w];
    FILTER_CHN_LOOP:
    for (int f = 0; f < out_chn; f++){
        WINDOW filter_buffer;
        FILTER_BUFFER_LOOP:
        for (int row = 0; row < k_h; row++)
        	FILTER_BUFFER_INNER_LOOP:
            for (int col = 0; col < k_w; col++)
                filter_buffer.insert(filter[f * k_h * k_w + row * k_w + col], row, col);


        LINEBUFFER line_buffer;
        WINDOW window_buffer;

        IMAGE_CHN_LOOP:
        for (int k = 0; k < in_chn; k++){
            IMAGE_H_LOOP:
            for (int i = 0; i < in_h; i++) {
                IMAGE_W_LOOP:
				for (int j = 0; j < in_w; j++) {
#pragma AP pipeline II=4
                    line_buffer.shift_up(j);
                    data_64 temp2 = data_64(input[k * in_h * in_w + i * in_w + j]);
                    //data_64 temp2 = (f == 0) ? data_64(input[k * in_h * in_w + i * in_w + j]) : inputBuf[k][i][j];
                    //if (f == 0) inputBuf[k][i][j] = temp2;
                    line_buffer.insert_bottom(temp2, j);
                    window_buffer.shift_right();
                    WINDOW_LOOP:
                    for (int rindex = 0; rindex < k_h; rindex++) {
                        window_buffer.insert(line_buffer.getval(rindex, j), rindex, k_w - 1);
                    }
                    if (i >= k_h - 1 && j >= k_w - 1) {
                        short p = f * (in_h - k_h + 1) * (in_w - k_w + 1) + (i - k_h + 1) * (in_w - k_w + 1) + (j - k_w + 1);
                        data_64 out_val;
                        FILTER_KERNEL_LOOP:
						for (int fc = 0; fc < BITWIDTH; fc++)
							INPUT_CHN_LOOP:
							for (int ic = 0; ic < BITWIDTH; ic++)
								FILTER_ROW_LOOP:
								for (int i = 0; i < k_h; i++)
									FILTER_COL_LOOP:
									for (int j = 0; j < k_w; j++){
										out_val.a[fc] = out_val.a[fc] + window_buffer.getval(i,j).a[ic] * filter_buffer.getval(i,j).a[fc];

									}
						output[p] += out_val;//conv2d_operator_64<k_h, k_w>(&window_buffer, &filter_buffer);
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
        input.a[i] = input.a[i] > data_16f(0) ? input.a[i] : data_16f(0);
    return input;
}


void relu_layer(data_64* input, short in_chn, short in_h, short in_w){
    for (int i = 0; i < in_chn; i++)
        for (int j = 0; j < in_h; j++)
            for (int k = 0; k < in_w; k++)
                input[i * in_h * in_w + j * in_w + k] = relu(input[i * in_h * in_w + j * in_w + k]);
}
#pragma SDS data access_pattern(image:SEQUENTIAL, filter:SEQUENTIAL, output:SEQUENTIAL)
void conv2d_wrapper(data_pack<data_16f> image[4*121], data_pack<data_16f> filter[16*9], data_pack<data_16f> output[16*81], short stride, short padding){
	data_64 filterBuf[16*9], outputBuf[16*81];
//	data_pack<data_16f> imageBuf[121];

//    for(int i=0; i<121; i++) {
//#pragma HLS PIPELINE
//        imageBuf[i] = image[i];
//    }
    for(int i=0; i<9; i++) {
#pragma HLS PIPELINE
        filterBuf[i] = filter[i];
    }

	conv2d<4,11,11,16,3,3,9,9>(image, filterBuf, outputBuf, 3, 3);
	for (int i=0; i < 16*81; i++){
#pragma HLS PIPELINE
		output[i].a0 = outputBuf[i].a[0];
		output[i].a1 = outputBuf[i].a[1];
		output[i].a2 = outputBuf[i].a[2];
		output[i].a3 = outputBuf[i].a[3];

	}
}

int main(){
    short in_ch = 1, in_h = 11, in_w = 11,
          out_ch = 1, k_h = 3, k_w = 3,
          stride = 3, padding = 3;
    short out_h = (in_h + 2 * padding - k_h)/stride + 1;
    short out_w = (in_w + 2 * padding - k_w)/stride + 1;

    data_pack<data_16f> image[4 * 11 * 11];
    data_pack<data_16f> filter[16 * 3 * 3];
    data_pack<data_16f> output[16 * 9 * 9];

#pragma AP data_pack variable=image
#pragma AP data_pack variable=filter
#pragma AP data_pack variable=output

                /* in_chn, in_h, in_w,
                 out_chn, k_h,  k_w,
                 stride, padding
                */
    init_layers_64(image, filter, 4, 11, 11, 16, 3, 3);
    conv2d_wrapper(image, filter, output, 3, 3);
    //conv2d<1,11,11,1,3,3,9,9>(image, filter, output, 3, 3);
    //deconv2d<7,7,7,7>(image, filter, output, 1, 1, 3, 3);
    for (int i = 0; i < 16 * 9 * 9; i++){
        if (i % 9 == 0)
            printf("\n");
        printf("%.2f ", float(output[i].a0));
    }


    return(0);
}


