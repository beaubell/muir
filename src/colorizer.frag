uniform sampler2D tex;

void main() {
   vec4 texel = texture2D(tex,gl_TexCoord[0].st);
   vec4 color;

   // Just take the red for intensity since they are all the same for grayscale.
   float i = texel.r;
   //float g = texel.g;
   //float b = texel.b;

   if (i < 0.25 )
      color = vec4(0.0,0.0,i * 4.0,0.0);
   else if (i < 0.5 )
      color = vec4(0.0,(i-0.25)* 4.0,1.0,0.0);
   else if (i < 0.75 )
       color = vec4((i-0.50)* 4.0,1.0,1.0-(i-0.50)* 4.0,0.0);
   else
       color = max(vec4(1.0,1.0-(i-0.75)* 4.0,0.0,0.0),0.0);
   gl_FragColor = color;
}