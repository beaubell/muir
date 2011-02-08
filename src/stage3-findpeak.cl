__kernel void
findpeak(__global float2* postfft_data,
         __global float*  output_data,
                  uint    range,
                  uint    num_rangebins)
{
  uint frame = get_global_id(0);
  unit frameidx = frame*num_rangebins;
  
  float max_sample = -MAXFLOAT;

  for(float i = 0; i < num_rangebins; i++)
  {
    float data = pown(postfft_data[frameidx + i].s0,2) + pown(postfft_data[frameidx + i].s1,2)

    if (data > max_sample)
        max_sample = data;
  }

  // Output data
  out_data[frameidx + range];

} 

