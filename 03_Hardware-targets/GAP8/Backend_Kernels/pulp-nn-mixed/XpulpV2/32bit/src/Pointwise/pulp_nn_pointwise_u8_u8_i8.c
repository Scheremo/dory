/*
 * pulp_nn_pointwise_u8_u8_i8.c
 * Nazareno Bruschi <nazareno.bruschi@unibo.it>
 *
 * Copyright (C) 2019-2020 University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pmsis.h"
#include "pulp_nn_utils.h"
#include "pulp_nn_kernels.h"


void pulp_nn_pointwise_u8_u8_i8(
          const uint8_t *pInBuffer,
          const uint16_t dim_in_x,
          const uint16_t dim_in_y,
          const uint16_t ch_in,
          const int8_t *pWeight,
          const uint16_t ch_out,
          const uint16_t dim_kernel_x,
          const uint16_t dim_kernel_y,
          const uint16_t padding_y_top,
          const uint16_t padding_y_bottom,
          const uint16_t padding_x_left,
          const uint16_t padding_x_right,
          const uint16_t stride_x,
          const uint16_t stride_y,
          const int8_t *bias,
          const uint16_t bias_shift,
          const int8_t out_shift,
          const uint16_t out_mult,
          uint8_t *pOutBuffer,
          const uint16_t dim_out_x,
          const uint16_t dim_out_y,
          int32_t *k,
          int32_t *lambda,
          uint8_t *pIm2ColBuffer,
          int flag_relu,
          int flag_batch_norm,
          unsigned int * memory_chan
) {
  uint16_t ch_in_r = ch_in;
  uint16_t ch_out_r = ch_out;

  int core_id = pi_core_id();
  int i_out_y, i_out_x, i_ker_y, i_ker_x;
  int Log2Core;

  uint8_t extra_chunk = ((dim_out_y & (NUM_CORES-1)) != 0);
  uint8_t extra_chunk_r;
  uint16_t dim_out_x_r;
  uint8_t section;
  int core_id_r;

  if(extra_chunk && dim_out_x > 1)
  {
    Log2Core = log2(NUM_CORES >> 1);
    core_id_r = (core_id >> 1);
    dim_out_x_r = (dim_out_x >> 1);
    section = (core_id & 0x1);
    extra_chunk_r = ((dim_out_y & ((NUM_CORES >> 1) - 1)) != 0);
  }
  else
  {
    Log2Core = log2(NUM_CORES);
    core_id_r = core_id;
    dim_out_x_r = dim_out_x;
    section = 0;
    extra_chunk_r = extra_chunk;
    extra_chunk = 0;
  }

  uint8_t flag_dim_out_x_odd = dim_out_x & 0x01;

  int chunk = (dim_out_y >> Log2Core) + extra_chunk_r;

  int start_pixel = min((chunk * core_id_r), dim_out_y);
  int stop_pixel = min(start_pixel + chunk, dim_out_y);

  uint8_t *pOut = pOutBuffer + (start_pixel * ch_out_r * dim_out_x) + (section * ch_out_r * dim_out_x_r);

  for (i_out_y = start_pixel; i_out_y < stop_pixel; i_out_y++)
  {
    i_out_x= (section * dim_out_x_r);

    for(int n = 0; n<(dim_out_x_r + (section * flag_dim_out_x_odd)); n++)
    {
      if((n & 0x0001) != 0)
      {
          uint8_t *pIm2Col = (pInBuffer + (i_out_x * ch_in) + (i_out_y * dim_in_x * ch_in));
          pOut = pulp_nn_matmul_u8_i8(
              pWeight,
              pIm2Col,
              ch_out,
              ch_in,
              bias_shift,
              out_shift,
              out_mult,
              k,
              lambda,
              bias,
              pOut,
              flag_relu,
              flag_batch_norm
              );
          i_out_x+=2;
      }
    }

    if((dim_out_x_r & 0x0001) != 0)
    {
      const int8_t *pA = pWeight;
      int i;
      int32_t * k1 = k;
      int32_t * lambda1 = lambda;
      for(i = 0; i < ch_out; i++)
      {
        int sum = 0;//((int)(bias[i]) << bias_shift);// + nn_round(out_shift);

        uint8_t *pB = (pInBuffer + (i_out_x * ch_in) + (i_out_y * dim_in_x * ch_in));
        uint16_t col_cnt_im2col = ch_in * dim_kernel_x * dim_kernel_y >> 2;
        for(int j=0; j < col_cnt_im2col; j++)
        {
          v4s inA = *((v4s*) pA);
          v4u inB = *((v4u*) pB);

          sum = SumDotp4(inB, inA, sum);
          pA+=4;
          pB+=4;
        }
        col_cnt_im2col = (ch_in * dim_kernel_y * dim_kernel_x) & 0x3;
        while (col_cnt_im2col)
        {
          int8_t inA1 = *pA++;
          uint8_t inB1 = *pB++;
          asm volatile("": : :"memory");
          sum += inA1 * inB1;

          col_cnt_im2col--;
        }
        if (flag_batch_norm && flag_relu)
        {
          *pOut = pulp_nn_bn_quant_u8(sum, *k1, *lambda1, out_shift);
          k1++;
          lambda1++;
          pOut++;
        }
        else
        {
          if(flag_relu == 1)
          {
            *pOut = pulp_nn_quant_u8(sum, out_mult, out_shift);
            pOut++;
          }
          else
          {
            *pOut = (uint8_t) clip8(sum >> out_shift);
            pOut++;
          }
        }
      }
    }
    pOut+=(extra_chunk * ((dim_out_x_r + ((1 - section) * flag_dim_out_x_odd)) * ch_out));
  }
  pi_cl_team_barrier(0);
}
