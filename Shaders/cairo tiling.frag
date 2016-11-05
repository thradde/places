//Cairo tiling by nimitz (stormoid.com) (twitter: @stormoid)

//Inspired by Petri Leskinen's "Cairo Pentagonal Tiling" http://pixelero.wordpress.com/page/2/

/*
	Cairo pentagonal tiling made using a "voronoi-like" function,
	the main difference is that half the tiles are reflected and the centers are
	displaced symmetrically.

	Please let me know if you can think of a simple way to get all the edges.
*/

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

float hash( float n ){ return fract(sin(n)*43758.5453);}


//returns: x -> distance form center | y -> cell id | z -> distance from edge
vec3 field(const in vec2 p)
{
    vec2 fp = fract(p);
    vec2 ip = floor(p);
    
    vec3 rz = vec3(1.);
    
    //vary the offset over time
    float of = sin(time*0.6)*.5;
    
    //reflect
   	float rf = mod(ip.x+ip.y,2.0);
    fp.x = rf-fp.x*sign(rf-.5);
    
	for(float j=0.; j<=1.; j++)
	for(float i=0.; i<=1.; i++)
    {
        vec2 b = vec2(j, i);
        
        //Displace each sample
        float sgn = sign(j-0.5); 
        float cmp = float(j == i);
        vec2 o = vec2(sgn*cmp,-sgn*(1.-cmp));
        //o = b-vec2(i,j); //variation
        vec2 sp = fp - b + of*o;
        b += o;
        
        float d = dot(sp,sp);
        
        if( d<rz.x )
        {
            rz.z = rz.x;
            rz.x = d;
            b.x = rf-b.x*sign(rf-.5);
        	rz.y = hash( dot(ip+b,vec2(7.0,113.0) ) );
        }
        else if( d<rz.z )
		{
            rz.z = d;
		}
    }
    
    //truncation
    float d = dot(fp-.5,fp-.5);
    d += 0.4; //truncation weight
    if (d < rz.x)
    {
        rz.z = rz.x;
        rz.x = d;
        rz.y = hash( dot(ip,vec2(7.0,113.0) ) );
    }
    else if(d < rz.z )
	{
    	rz.z = d;
	}
    
    //F2-F1 edge detection
    rz.z = rz.z-rz.x;
    
    return rz;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 p = fragCoord.xy / iResolution.xy;
	p.x *= iResolution.x/iResolution.y;
    float a = sin(time)*.5;
    float b = .5;
    
    p *= 6.+sin(time*0.4);
    p.x += time;
    p.y += sin(time)*0.5;
    
    vec3 rz = field(p);
    
    vec3 col = (sin(vec3(.2,.55,.8)+rz.y*4.+2.)+0.4)*0.6+0.5;
    col *= 1.-rz.x;
    col *= smoothstep(0.,.04,rz.z);
    
    fragColor = vec4(col,1.0);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
