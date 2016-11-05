#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;


float CausticDistortDomainFn(vec2 pos)
{
	pos.x *= (pos.y * .20 + .5);
	//pos.x *= 1. + sin(time / 1.) / 10.;
	return pos.x;
}


vec4 function(vec2 p)
{
	float x = CausticDistortDomainFn(p) * .75; // 10 .. 30
	//  * (sin(time / 2) * .1 + 1.1)
	
	// gut: float fx = sin((x+.5)*3 * (sin(time / 2) * 2 + 2)) + sin((x+.5)*6*16.3);
//	float fx = sin((x+.5)*3 * (sin(time / 2) * 2 + 1)) + sin((x+.5)*6*16.3 * (sin(time / 3) * .1 + 1.1));
	float fx = sin((x+.5)*3 * (sin(time / 2.3) * 1.2 + 1)) + sin((x+.5)*6*12.3 * (sin(time / 3) * .1 + 1));
	// float fx = sin(x) * 2 + sin(x * 16.3 * (sin(time / 2) * .5 + 2));
	fx = fx * fx / 3;
	
	//fx *= (sin((p.x + .5) * 3.14) + 1) / 2;	// shade
		//fx *= pow(p.y + .75, 3);
	fx *= p.y; // clamp(p.y, .2, 1);
//fx += (sin((p.x + .5) * 3.14) + 1) / 2;	// shade
fx += (sin((p.x + .5) * 3.14) + 1) / 4 + .5;	// shade

//	return vec4(.1, .5, .7, 1) * fx;	// vec4(r, g, fx, 1.0);
	return vec4(.039, .23, .46, 1) * fx;	// vec4(r, g, fx, 1.0);
}

void main(void)
{
	vec2 position = gl_FragCoord.xy / resolution;
	position.x -= .5;
	gl_FragColor = function(position);
}