__kernel void
findpeak(__global float2* postfft_data,
         __global float*  output_data,
                  uint    range,
                  uint    num_rangebins,
                  float   normalize)
{
  uint frame = get_global_id(0);
  uint frameidx = frame*num_rangebins;
  
  float max_sample = -INFINITY;

  for(uint i = 0; i < num_rangebins; i++)
  {
    float data = pown(postfft_data[frameidx + i].s0,2) + pown(postfft_data[frameidx + i].s1,2);

    max_sample = max(data, max_sample);
  }

  // Output data
  output_data[frameidx + range] = sqrt(max_sample)*normalize;

} 

