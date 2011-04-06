__kernel void
findpeak(__global float*  power_data,
         __global float*  output_data,
           const  uint    range,
           const  uint    fft_size,
           const  uint    in_stride,
           const  uint    out_stride,
           const  float   normalize)
{
  uint inframe  = get_global_id(0)*in_stride;
  uint outframe = get_global_id(0)*out_stride;
  
  float max_sample = -INFINITY;

  for(uint i = 0; i < fft_size; i++)
  {
    max_sample = max(power_data[inframe + i], max_sample);
  }

  // Output data
  output_data[outframe + range] = sqrt(max_sample)*normalize;

} 

