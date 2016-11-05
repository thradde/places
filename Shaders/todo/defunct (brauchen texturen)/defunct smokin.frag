//Smokin' by nimitz (@stormoid) (Looks better in fullscreen)
// 


==> Funktioniert nicht!
	Benötigt eine 256 x 256 Pixel Texture b/w noise,  gebunden an Variable iChannel0 

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

void mainImage( out vec4 f, in vec2 w ){
    vec4 p = vec4(w,0.,1.)/iResolution.xyxx*6.-3.,z = p-p, c, d=z;
	float t = iGlobalTime;
    p.x -= t*.4;
    for(float i=0.;i<8.;i+=.3)
        c = texture2D(iChannel0, p.xy*.0029)*11.,
        d.x = cos(c.x+t), d.y = sin(c.y+t),
        z += (2.-abs(p.y))*vec4(.1*i, .3, .2, 9),
        //z += (2.-abs(p.y))*vec4(.2,.4,.1*i,1), // Alt palette
        z *= dot(d,d-d+.03)+.98,
        p -= d*.022;
    
	f = z/25.;
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
