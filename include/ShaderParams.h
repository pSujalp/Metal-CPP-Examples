#pragma once

typedef struct
{
    unsigned int row_dim_x; // Number of rows in X
    unsigned int col_dim_x; // Number of columns in X
    unsigned int inner_dim; // Number of columsn in A = number of rows in B
} MatMulParams;

