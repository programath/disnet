#include 

// stream winograd
template<typename T, int m, int r>
void winograd(T input[in_c * in_h * in_w], 
			  T filter[out_c * k_h * k_w], 
			  T output[out_c * out_h * out_w]){
	short tile_w = m + r - 1;
	short tile_h = m + r - 1;
	LINEBUFFER line_buffer;
	WINDOW window_buffer;
	// prepare tile
	for (int k = 0; k < in_c; k++){
		for (int i = 0; i < in_h; i++){
			for (int j = 0; j < in_w; j++){
				line_buffer.shift_up(j);
				T temp = input[k * in_h * in_w + i * in_w + j];
				line_buffer.insert_buttom(temp, j);
				window_buffer.shift_right();
				for (int rindex = 0; rindex < tile_h; rindex++){
					window_buffer.insert(line_buffer.getval(rindex, j), rindex, tile_w - 1);
				}
				if (j >= tile_w - 1 && (j - r) % m == 0 && i >= tile_h - 1 && (i - r) % m == 0){
					//direct product
					G = [1 0 0; 0.5 0.5 0.5; 0.5 -0.5 0.5; 0 0 1];
					U[4][4];
					V[4][4];
					for (int u = 0; u < 4; u++){
						for (int v = 0; v < 4; v++){
							M[u][v] = U[u][v] * V[u][v];
							if (v != 3)	
								temp[u][0] = temp[u][0] + M[u][v];
							if (v == 1)
								temp[u][1] = temp[u][1] + M[u][v];
							else if (v & 0x10 != 0 )
								temp[u][1] = temp[u][1] - M[u][v];			
						}
					}
					//M=U*V;
					Y[h][w] += temp[0][0] + temp[1][0] + temp[2][0];
					Y[h][w+1] += temp[0][1] + temp[1][1] + temp[2][1];
					Y[h+1][w] += temp[1][0] - temp[2][0] - temp[3][0];
					Y[h+1][w+1] += temp[1][1] - temp[2][1] - temp[3][1];
					w = w + 2;
					if (w >= out_w - 1) {
						w = 0;
						h = h + 2;
						if (h >= out_h - 1) h = 0;
					}			
				}
			}		
		}
	}
}


// linebuffer become a 3d structure and every time it read a channel on one pixel
// BRAM winograd
template<typename T, int m, int r>
void winograd(T input[in_h * in_w * in_c], 
			  T filter[k_h * k_w * out_c], 
			  T output[out_h * out_w * out_c]){
	short tile_w = m + r - 1;
	short tile_h = m + r - 1;
	for (int k = 0; k < in_h; k++)
		for (int i = 0; i < in_w; i++)
			for (int j = 0; j < in_c; j++)
				inputBuf[k][i][j] = *input++;
	for (int k = 0; k < out_h; k++)
		for (int i = 0; i < out_w; i++)
			for (int j = 0; j < out_c; j++)
				filterBuf[k][i][j] = *filter++;



	LINEBUFFER_3Dline_buffer;
	WINDOW_3D window_buffer;
	T Utemp[4][2], U[4][4],V[4][4], Vtemp[4][4], M[4][4];

	Utemp[0][0] = filterBuf[0][0][l] + filterBuf[0][1][l] + filterBuf[0][2][l];
	Utemp[0][1] = filterBuf[0][0][l] - filterBuf[0][1][l] - filterBuf[0][2][l];
	Utemp[1][0] = filterBuf[1][0][l] + filterBuf[1][1][l] + filterBuf[1][2][l];
	Utemp[1][1] = filterBuf[1][0][l] - filterBuf[1][1][l] - filterBuf[1][2][l];
	Utemp[2][0] = filterBuf[2][0][l] + filterBuf[2][1][l] + filterBuf[2][2][l];
	Utemp[2][1] = filterBuf[2][0][l] - filterBuf[2][1][l] - filterBuf[2][2][l];

	Utemp[3][0] = filterBuf[0][0][l] + filterBuf[1][0][l] + filterBuf[2][0][l];
	Utemp[3][1] = filterBuf[0][0][l] - filterBuf[1][0][l] + filterBuf[2][0][l];
	
	U[0][0] = filterBuf[0][0][l];
	U[0][1] = Utemp[0][0] * 0.5;
	U[0][2] = Utemp[0][1] * 0.5;
	U[0][3] = filterBuf[0][2][l];

	U[1][0] = (filterBuf[0][0][l] + filterBuf[1][0][l] + filterBuf[2][0][l])*0.5;
	U[1][1] = (Utemp[0][0] + Utemp[1][0] + Utemp[2][0])*0.25;
	U[1][2] = (Utemp[0][1] + Utemp[1][1] + Utemp[2][1])*0.25;
	U[1][3] = (filterBuf[0][2][l] + filterBuf[1][2][l] + filterBuf[2][2][l])*0.5;
	
	U[2][0] = (filterBuf[0][0][l] - filterBuf[1][0][l] + filterBuf[2][0][l])*0.5;
	U[2][1] = (Utemp[0][0] - Utemp[1][0] + Utemp[2][0])*0.25;
	U[2][2] = (Utemp[0][1] - Utemp[1][1] + Utemp[2][1])*0.25;
	U[2][3] = (filterBuf[0][2][l] - filterBuf[1][2][l] + filterBuf[2][2][l])*0.5;
	
	U[3][0] = filterBuf[2][0][l];
	U[3][1] = Utemp[2][0] * 0.5;
	U[3][2] = Utemp[2][1] * 0.5;
	U[3][3] = filterBuf[2][2][l];
	// prepare tile
	for (int i = 0; i < in_h; i++){
		for (int j = 0; j < in_w; j++){
			// all channel shift up
			for (int k = 0; k < in_c; k++){
				line_buffer.shift_up(j,k);
				T temp = inputBuf[i][j][k];
				line_buffer.insert_buttom(temp, j, k);
				window_buffer.shift_right(k);//
				for (int rindex = 0; rindex < tile_h; rindex++){
					window_buffer.insert(line_buffer.getval(rindex, j , k), rindex, tile_w - 1, k);
				}
				if (j >= tile_w - 1 && (j - r) % m == 0 && i >= tile_h - 1 && (i - r) % m == 0){
					//direct product

					Vtemp[0][0] = window_buffer.getval(0,0,k);
					Vtemp[0][1] = window_buffer.getval(0,1,k) - window_buffer.getval(0,2,k) + window_buffer.getval(0,3,k);
					Vtemp[0][2] = -window_buffer.getval(0,0,k) + window_buffer.getval(0,1,k) + window_buffer.getval(0,2,k);
					Vtemp[0][3] = -window_buffer.getval(0,3,k);

					Vtemp[1][0] = window_buffer.getval(1,0,k);
					Vtemp[1][1] = window_buffer.getval(1,1,k) - window_buffer.getval(1,2,k) + window_buffer.getval(1,3,k);
					Vtemp[1][2] = -window_buffer.getval(1,0,k) + window_buffer.getval(1,1,k) + window_buffer.getval(1,2,k);
					Vtemp[1][3] = -window_buffer.getval(1,3,k);

					Vtemp[2][0] = window_buffer.getval(2,0,k);
					Vtemp[2][1] = window_buffer.getval(2,1,k) - window_buffer.getval(2,2,k) + window_buffer.getval(2,3,k);
					Vtemp[2][2] = -window_buffer.getval(2,0,k) + window_buffer.getval(2,1,k) + window_buffer.getval(2,2,k);
					Vtemp[2][3] = -window_buffer.getval(2,3,k);

					Vtemp[3][0] = window_buffer.getval(3,0,k);
					Vtemp[3][1] = window_buffer.getval(3,1,k) - window_buffer.getval(3,2,k) + window_buffer.getval(3,3,k);
					Vtemp[3][2] = -window_buffer.getval(3,0,k) + window_buffer.getval(3,1,k) + window_buffer.getval(3,2,k);
					Vtemp[3][3] = -window_buffer.getval(3,3,k);

					V[0][0] = Vtemp[0][0] - Vtemp[2][0];
					V[0][1] = Vtemp[0][1] - Vtemp[2][1];
					V[0][2] = Vtemp[0][2] - Vtemp[2][2];
					V[0][3] = Vtemp[0][3] - Vtemp[2][3];

					V[1][0] = Vtemp[1][0] + Vtemp[2][0];
					V[1][1] = Vtemp[1][1] + Vtemp[2][1];
					V[1][2] = Vtemp[1][2] + Vtemp[2][2];
					V[1][3] = Vtemp[1][3] + Vtemp[2][3];
					
					V[2][0] = -Vtemp[1][0] + Vtemp[2][0];
					V[2][1] = -Vtemp[1][1] + Vtemp[2][1];
					V[2][2] = -Vtemp[1][2] + Vtemp[2][2];
					V[2][3] = -Vtemp[1][3] + Vtemp[2][3];
					
					V[3][0] = Vtemp[1][0] - Vtemp[3][0];
					V[3][1] = Vtemp[1][1] - Vtemp[3][1];
					V[3][2] = Vtemp[1][2] - Vtemp[3][2];
					V[3][3] = Vtemp[1][3] - Vtemp[3][3];

					for (int u = 0; u < 4; u++){
						for (int v = 0; v < 4; v++){
							M[u][v] = U[u][v] * V[u][v];
							if (v != 3)	
								temp[u][0] = temp[u][0] + M[u][v];
							if (v == 1)
								temp[u][1] = temp[u][1] + M[u][v];
							else if (v & 0x10 != 0 )
								temp[u][1] = temp[u][1] - M[u][v];			
						}
					}
					//M=U*V;
					Y[l][h][w] += temp[0][0] + temp[1][0] + temp[2][0];
					Y[l][h][w+1] += temp[0][1] + temp[1][1] + temp[2][1];
					Y[l][h+1][w] += temp[1][0] - temp[2][0] - temp[3][0];
					Y[l][h+1][w+1] += temp[1][1] - temp[2][1] - temp[3][1];		
				}
			}
			if (j >= tile_w - 1 && (j - r) % m == 0 && i >= tile_h - 1 && (i - r) % m == 0){
				w = w + 2;
				if (w >= out_w - 1) {
					w = 0;
					h = h + 2;
					if (h >= out_h - 1) h = 0;
			}	
		}
	}
}


