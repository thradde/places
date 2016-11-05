
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

float r(float n)
{
 	return fract(cos(n*89.42)*343.42);
}
vec2 r(vec2 n)
{
 	return vec2(r(n.x*23.62-300.0+n.y*34.35),r(n.x*45.13+256.0+n.y*38.89)); 
}
float worley(vec2 P,vec2 R,float s)
{
    vec2 n = P+iGlobalTime*s-0.5*R;
    float dis = 64.0;
    for(int x = -1;x<2;x++)
    {
        for(int y = -1;y<2;y++)
        {
            vec2 p = floor(n/s)+vec2(x,y);
            float d = length(r(p)+vec2(x,y)-fract(n/s));
            if (dis>d)
            {
             	dis = d;   
            }
        }
    }
    return pow(dis,4.0);
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 c = fragCoord;
    vec2 r = iResolution.xy;
	vec3 col = vec3(worley(c,r,32.0)+worley(c,r,36.0)+worley(c,r,44.0));
	fragColor = vec4(col,1.0);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
