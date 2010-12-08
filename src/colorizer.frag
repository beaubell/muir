uniform sampler2D tex;
uniform float data_min;
uniform float data_max = 100.0;

void main() {
   vec4 texel = texture2D(tex,gl_TexCoord[0].st);
   vec4 color;

   // Just take the red for intensity since they are all the same for grayscale.
   float sample = texel.r;
   //float g = texel.g;
   //float b = texel.b;

   // Normalize sample
   float i = (sample-data_min)/(data_max-data_min);

   if (i < 0.25 )
      color = max(vec4(0.0,0.0,i * 4.0,0.0),0.0);
   else if (i < 0.5 )
      color = vec4(0.0,(i-0.25)* 4.0,1.0,0.0);
   else if (i < 0.75 )
       color = vec4((i-0.50)* 4.0,1.0,1.0-(i-0.50)* 4.0,0.0);
   else
       color = min(vec4(1.0,1.0-(i-0.75)* 4.0,0.0,0.0),1.0);
   gl_FragColor = color;
}