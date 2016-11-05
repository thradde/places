// by mrqzzz (https://www.shadertoy.com/user/mrqzzz)
// 
#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    float t=iGlobalTime/13. + cos(iGlobalTime/2.);
	fragColor = vec4(
        0.5 + 0.5 * ( sin(uv.x*cos(t)*6.*cos(uv.y*t/232.)) + sin(uv.y*sin(t)*3.*cos(uv.x*t/212.)) ),
        0.5 + 0.5 * ( cos(uv.x*sin(t)*5.*cos(uv.y*t/211.)) + sin(uv.y*sin(t)*6.*cos(uv.x*t/234.)) ),
        0.5 + 0.5 * ( cos(uv.x*cos(t)*4.*cos(uv.y*t/311.)) + sin(uv.y*sin(t)*7.*cos(uv.x*t/321.)) ),
        1.);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
