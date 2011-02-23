__kernel void
phasecode(__global float2* sample_data,
          __constant float*  phasecode_data,
          __global float2* prefft_data,
                   uint    phasecode_offset, 
                   uint    phasecode_size, 
                   uint    num_rangebins)
{
  unsigned int range = get_global_id(0);
  unsigned int frame = get_global_id(1)*num_rangebins;

  if (range > phasecode_size || (phasecode_offset + range) > num_rangebins)
  {
      prefft_data[frame + range]   = 0.0f;
  }
  else
  {
      float phase = phasecode_data[range];
      prefft_data[frame + range]   = phase * sample_data[frame + range + phasecode_offset];
  }                  
} 

