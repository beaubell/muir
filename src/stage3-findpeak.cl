__kernel void
findpeak(__global float2* postfft_data,
         __global float*  output_data,
                  uint    range,
                  uint    num_rangebins,
                  float   normalize)
{
  uint frame = get_global_id(0);
  uint frameidx = frame*num_rangebins;
  
  float max_sample = -MAXFLOAT;

  for(uint i = 0; i < num_rangebins; i++)
  {
    float data = pown(postfft_data[frameidx + i].s0,2) + pown(postfft_data[frameidx + i].s1,2);

    if (data > max_sample)
        max_sample = data;
  }

  // Output data
  output_data[frameidx + range] = max_sample*normalize;

} 

