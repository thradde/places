
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy/iResolution.xy;
	float red = clamp(uv.y*2.0-1.0+cos(uv.x+iGlobalTime),0.0,1.0);
    float green = pow(1.0-abs(uv.y*2.0-cos(uv.x+iGlobalTime*0.7)),8.0);
    float blue = uv.y*0.2+pow(fract(cos(fragCoord.x*35.7375)*36764.86437+0.5+0.5*cos(iGlobalTime/5.0))*0.1,2.0);
	fragColor = vec4(red,green,blue,1.0);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
