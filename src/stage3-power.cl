__kernel void
power(__global float2* postfft_data,
      __global float*  power_data,
               uint    num_rangebins)
{
  unsigned int range = get_global_id(0);
  unsigned int frameidx = get_global_id(1)*num_rangebins;

  float data = pown(postfft_data[frameidx + range].s0,2) + pown(postfft_data[frameidx + range].s1,2);
  
  power_data[frameidx + range] = data;

} 

