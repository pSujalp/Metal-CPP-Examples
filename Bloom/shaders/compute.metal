
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;
using namespace simd;



kernel void BlurcomputeShader(texture2d<float, access::read> source [[ texture(0) ]],
  texture2d<float, access::write> dest [[ texture(2) ]],
  uint2 gid [[ thread_position_in_grid ]]){


    float4 result = source.read(gid);
    
    return  float4(col, 1.0); ;
}

kernel void GreyScale_computeShader(texture2d<float, access::read> source [[ texture(0) ]],
  texture2d<float, access::write> dest [[ texture(2) ]],
  uint2 gid [[ thread_position_in_grid ]]){
    float4 result = source.read(gid);
    float average = (result[0] + result[1] + result[2]) / 3.0;
    result = float4(average, average, average, 1.0);

    dest.write(result, gid);
}



kernel void ADDcomputeShader(texture2d<float, access::read> source [[ texture(0) ]],
  texture2d<float, access::read> mask [[ texture(1) ]],
  texture2d<float, access::write> dest [[ texture(2) ]],
  uint2 gid [[ thread_position_in_grid ]]){

  float4 source_color = source.read(gid);
  float4 mask_color = mask.read(gid);
  float4 result_color = source_color + mask_color;

  dest.write(result_color, gid);
}