__kernel void
phasecode(__global float2* sample_data,
          __constant float*  phasecode_data,
          __global float2* prefft_data,
                   uint    phasecode_offset, 
                   uint    phasecode_size, 
                   uint    num_rangebins,
                   uint    num_fft
         )
{
  unsigned int range = get_global_id(0);
  unsigned int frame_id = get_global_id(1);
  unsigned int inframe_idx  = mad24(frame_id, num_rangebins, range);
  unsigned int outframe_idx = mad24(frame_id, num_fft, range);

  if (range > phasecode_size || (phasecode_offset + range) > num_rangebins)
  {
      prefft_data[outframe_idx]   = 0.0f;
  }
  else
  {
      float phase = phasecode_data[range];
      prefft_data[outframe_idx]   = phase * sample_data[inframe_idx + phasecode_offset];
  }
} 

