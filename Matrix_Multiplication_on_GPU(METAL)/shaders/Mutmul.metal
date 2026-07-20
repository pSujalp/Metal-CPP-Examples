//
//  mat_mul_optimized_nv.metal
//  metal_performance_testing
//
//  Created by Brian Vogel on 2022/08/27.
//

#include <metal_stdlib>

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

kernel void mat_mul_naive(device const float* inA,
                           device const float* inB,
                           device float* result,
                           constant MatMulParams& params,
                           uint2 id [[thread_position_in_grid]])
{
    uint row = id.y;
    uint col = id.x;
    if (row >= params.row_dim_x || col >= params.col_dim_x) return;

    float sum = 0.0;
    for (uint k = 0; k < params.inner_dim; ++k)
        sum += inA[row * params.inner_dim + k] * inB[k * params.col_dim_x + col];

    result[row * params.col_dim_x + col] = sum;
}


kernel void mat_mul_simple1(device const float* A,
                            device const float* B,
                            device float* X,
                            constant MatMulParams& params,
                            uint2 id [[ thread_position_in_grid ]])
{
    // Note: matrices are in row-major order in the supplied backing arrays.
    const uint row_dim_x = params.row_dim_x;
    const uint col_dim_x = params.col_dim_x;
    const uint inner_dim = params.inner_dim;
    
    // Check if the thread is in-bounds.
    if ((id.x < col_dim_x) && (id.y < row_dim_x)) {
        // id.x is the column index of the result matrix.
        // id.y is the row index of the result matrix.
        const uint index = id.y*col_dim_x + id.x;
        float sum = 0;
        for (uint k = 0; k < inner_dim; ++k) {
            // index_A corresponds to A[id.y, k]
            const uint index_A = id.y*inner_dim + k;
            
            // index_B corresponds to B[k, id.x]
            const uint index_B = k*col_dim_x + id.x;

            sum += A[index_A] * B[index_B];
        }
        X[index] = sum;
    }
}


kernel void mat_mul_opt1(device const float* A,
                            device const float* B,
                            device float* X,
                            constant MatMulParams& params,
                            uint2 id [[ thread_position_in_grid ]])
{
    // Note: matrices are in row-major order in the supplied backing arrays.
    const uint row_dim_x = params.row_dim_x;
    const uint col_dim_x = params.col_dim_x;
    const uint inner_dim = params.inner_dim;
    const uint idx = id.x*4; // column index of the corner in X.
    const uint idy = id.y*4; // row index of the corner in X.
    // Note: float4x4 uses column major: Asub[m][n] is row n of column m.
    float4x4 Asub(0.0f);
    float4x4 Bsub(0.0f);
    float4x4 Xsub(0.0f);
    // bounds check can potentially be removed but does not seem to affect performance
    if ((idx < col_dim_x) && (idy < row_dim_x)) {
        uint k = 0;
        while (k < inner_dim) {
            // Read the values into 4x4 submatrices Asub and Bsub.
            for (uint j = 0; j < 4; ++j) { // column offset into X
                for (uint i = 0; i < 4; ++i) { // row offset into X
                    // corresponds to A[idy + i, k + j]
                    Asub[j][i] = A[(idy + i)*inner_dim + k + j];
                    // corresponds to B[k + i, idx + j]
                    Bsub[j][i] = B[(k + i)*col_dim_x + idx + j];
                }
            }
            // Multiply the two 4x4 submatrices and accumulate the result.
            Xsub += Asub * Bsub;
            k += 4;
        }
        // Write out the results.
        for (uint j = 0; j < 4; ++j) { // column offset into X
            for (uint i = 0; i < 4; ++i) { // row offset into X
                X[(idy + i)*col_dim_x + idx + j] = Xsub[j][i];
            }
        }
    }
}


kernel void mat_mul_opt2(device const float* A,
                            device const float* B,
                            device float* X,
                            constant MatMulParams& params,
                            uint2 id [[ thread_position_in_grid ]])
{
    // Note: matrices are in row-major order in the supplied backing arrays.
    const uint row_dim_x = params.row_dim_x;
    const uint col_dim_x = params.col_dim_x;
    const uint inner_dim = params.inner_dim;
    const uint idx = id.x*4; // column index of the corner in X.
    const uint idy = id.y*8; // row index of the corner in X.
    // Note: float4x4 uses column major: Asub[m][n] is row n of column m.
    float4x4 Asub(0.0f);
    float4x4 Bsub(0.0f);
    float4x4 Xsub(0.0f);
    float4x4 Asub2(0.0f);
    float4x4 Xsub2(0.0f);
    // bounds check can potentially be removed but does not seem to affect performance
    if ((idx < col_dim_x) && (idy < row_dim_x)) {
        uint k = 0;
        while (k < inner_dim) {
            // Read the values into the 4x4 submatrices.
            for (uint i = 0; i < 4; ++i) { // row offset into X
                for (uint j = 0; j < 4; ++j) { // column offset into X
                    // corresponds to A[idy + i, k + j]
                    Asub[j][i] = A[(idy + i)*inner_dim + k + j];
                }
            }
            for (uint i = 0; i < 4; ++i) { // row offset into X
                for (uint j = 0; j < 4; ++j) { // column offset into X
                    // corresponds to B[k + i, idx + j]
                    Bsub[j][i] = B[(k + i)*col_dim_x + idx + j];
                }
            }
            for (uint i = 0; i < 4; ++i) { // row offset into X
                for (uint j = 0; j < 4; ++j) { // column offset into X
                    // corresponds to A[idy + i + 4, k + j]
                    Asub2[j][i] = A[(idy + i + 4)*inner_dim + k + j];
                }
            }
            // Multiply the 4x4 submatrices and accumulate the result.
            Xsub += Asub * Bsub;
            Xsub2 += Asub2 * Bsub;
            k += 4;
        }
        // Write out the results.
        for (uint i = 0; i < 4; ++i) { // row offset into X
            for (uint j = 0; j < 4; ++j) { // column offset into X
                X[(idy + i)*col_dim_x + idx + j] = Xsub[j][i];
                X[(idy + i + 4)*col_dim_x + idx + j] = Xsub2[j][i];
            }
        }
    }
}
