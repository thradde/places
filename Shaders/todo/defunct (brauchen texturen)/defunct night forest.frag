// night forest by fizzer (https://www.shadertoy.com/user/fizzer)
// 

==> uses iDate.w


#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

void mainImage( out vec4 f, in vec2 w )
{
    vec2 q=w/iResolution.xy,p;
    vec4 o=mix(vec4(.6,0,.6,1),vec4(1,1,.3,1),q.y);

    for(float i=8.;i>0.;--i)
    {
        p=q*i;
        p.x+=iDate.w *.3;

        p=cos(p.x+vec2(0,1.6))*sqrt(p.y+1.+cos(p.x+i))*.7;

        for(int j=0;j<20;++j)
            p=reflect(p,p.yx)+p*.14;

        o=f=dot(p,p)<3.?o-o:o;
    }
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
