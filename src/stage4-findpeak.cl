__kernel void
findpeak(__global float*  power_data,
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
    //float data = power_data[frameidx + i];

    max_sample = max(power_data[frameidx + i], max_sample);
  }

  // Output data
  output_data[frameidx + range] = sqrt(max_sample)*normalize;

} 

