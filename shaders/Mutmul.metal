
#include <metal_stdlib>
#include "ShaderParams.h"
using namespace metal;
 
typedef struct
{
    unsigned int row_dim_x; // Number of rows in X
    unsigned int col_dim_x; // Number of columns in X
    unsigned int inner_dim; // Number of columsn in A = number of rows in B
} MatMulParams;



kernel void mat_mul_optimized_nv(device const float* inA,
                                 device const float* inB,
                                 device float* result,
                                 constant MatMulParams& params,
                                 uint2 threadgroup_pos [[ threadgroup_position_in_grid ]],
                                 uint2 local_thread_idx [[ thread_position_in_threadgroup ]],
                                 uint2 id [[ thread_position_in_grid ]])
{
    
    // Note: be sure that this is set to the same value as "threads per group" in the calling code!
    const int BLOCK_SIZE = 8;

    const uint wB = params.col_dim_x;
    const uint wA = params.inner_dim;
    
    // Block index
    const uint bx =threadgroup_pos.x;
    const uint by = threadgroup_pos.y;
    
    // Thread index
    const uint tx =local_thread_idx.x;
    const uint ty =local_thread_idx.y;
    
    // Index of the first sub-matrix of A processed by the block
    const uint aBegin = wA * BLOCK_SIZE * by;

    // Index of the last sub-matrix of A processed by the block
    const uint aEnd   = aBegin + wA - 1;

    // Step size used to iterate through the sub-matrices of A
    const uint aStep  = BLOCK_SIZE;

    // Index of the first sub-matrix of B processed by the block
    const uint bBegin = BLOCK_SIZE * bx;

    // Step size used to iterate through the sub-matrices of B
    const uint bStep  = BLOCK_SIZE * wB;

    // Csub is used to store the element of the block sub-matrix
    // that is computed by the thread
    float Csub = 0;
    
    // Loop over all the sub-matrices of A and B
    // required to compute the block sub-matrix
    for (uint a = aBegin, b = bBegin;
        a <= aEnd;
        a += aStep, b += bStep) {
        // Declaration of the shared memory array As used to
        // store the sub-matrix of A
        threadgroup float As[BLOCK_SIZE][BLOCK_SIZE];
        

        // Declaration of the shared memory array Bs used to
        // store the sub-matrix of B
        threadgroup float Bs[BLOCK_SIZE][BLOCK_SIZE];

        // Load the matrices from device memory
        // to shared memory; each thread loads
        // one element of each matrix
        As[ty][tx] = inA[a + wA * ty + tx];
        Bs[ty][tx] = inB[b + wB * ty + tx];

        // Synchronize to make sure the matrices are loaded
        threadgroup_barrier(mem_flags::mem_none);

        // Multiply the two matrices together;
        // each thread computes one element
        // of the block sub-matrix
        for (int k = 0; k < BLOCK_SIZE; ++k) {
            Csub += As[ty][k] * Bs[k][tx];
        }

        // Synchronize to make sure that the preceding
        // computation is done before loading two new
        // sub-matrices of A and B in the next iteration
        threadgroup_barrier(mem_flags::mem_none);
      }

    const int c = wB * BLOCK_SIZE * by + BLOCK_SIZE * bx;
    result[c + wB * ty + tx] = Csub;    
}