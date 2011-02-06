__kernel void
phasecode(__global float2* sample_data,
          __global float* phasecode_data,
          __global float2* prefft_data, 
                   uint   phase_offset, 
                   uint   phase_size, 
                   uint   num_rangebins)
{
  unsigned int range = get_global_id(0)%num_rangebins;
  //unsigned int frame = get_global_id(0)/num_rangebins*num_rangebins;
  unsigned int frame = get_global_id(1)*num_rangebins;

  if (range < phase_offset || range >= (phase_offset+phase_size))
  {
      prefft_data[frame + range]   = 0.0f;
  }
  else
  {
      float phase = phasecode_data[range-phase_offset];
      prefft_data[frame + range]   = phase * sample_data[frame + range];
  }                  
} 

