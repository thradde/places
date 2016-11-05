// https://www.shadertoy.com/view/Xd2XRh
// Uploaded by abubusoft in 2014-Aug-25

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

const float VELOCITY        = 1.0  ;           // speed of lines [ 0.5  .. 1.5  ] =  1.0
const float HEIGHT          = 0.5    ;           // height of the lines  [ 0    .. 1.0  ] =  0.5
const float FREQUENCY       = 7.5 ;           // frequency  [ 1.0  .. 14.0 ] =  9.0
const float AMPLITUDE       = 0.3 ;           // amplitude  [ 0.1  .. 0.5  ] =  0.2
const int   NUMBER          = 10    ;           // lines      [ 0    .. 20   ] = 10.0
const float INVERSE         = 1.0 / float(10);  // inverse

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   vec3 col = vec3( 1.);
   
   float rColMod;
   float gColMod;
   float bColMod;
   
   float offset;
   float t;
   
   float color;
   float colora;
   
   float tsin;
           
   for (int i = 0; i < NUMBER; ++i)
   {
      vec2 pos= fragCoord.xy/iResolution.xy;
      
      offset = float(i) * INVERSE;
            
      t      = iGlobalTime * 0.25 + VELOCITY *(offset * offset * 2.);
      
      tsin   = sin( t );
      
      pos.y -= HEIGHT;
      pos.y+=sin(pos.x * FREQUENCY + t ) * AMPLITUDE * tsin;
      
      color  = 1.0 - pow( abs( pos.y ) , 0.2 );
      colora = pow( 1. , 0.2 * abs( pos.y ) );
      
      rColMod = (1. - (offset * .5) + .5) * colora ;
      gColMod = ((offset * .5) + .5) * colora ;
      bColMod = ((offset * .5) + .5) * colora ;
           
      col -= color * INVERSE * vec3( mix(rColMod, gColMod, tsin), mix(gColMod, bColMod, tsin) , mix(bColMod, rColMod, tsin)) ;      
   }
   
   fragColor=vec4(col.x, col.y, col.z ,1.0);
}


void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
