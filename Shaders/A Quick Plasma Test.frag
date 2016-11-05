
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iGlobalTime time
#define iResolution resolution
#define iMouse mouse

/*
 * This function will function like an array.
 */
vec2 getWaveSource(int ws)
{
	vec2 outp;
	if (ws == 0)
	{
		outp = vec2(-100,-100);
	}
	else if (ws == 1)
	{
		outp = vec2(-100,500);
	}
	else
	{
		outp = vec2(500,-500);
	}
	return outp;
}
/*
 * Don't need an expensive square root operation.
 * This returns distance squared, not distance.
 */
float distanceSq(vec2 a, vec2 b)
{
	vec2 diff = a - b;
	return dot(diff, diff);
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	const int wsCount = 3;
	
	vec2 uv = fragCoord.xy / iResolution.xy;
	
	float wavePower = 0.0;
	for(int i=0; i<wsCount; i++)
	{
		vec2 src = getWaveSource(i);
		float dist = distanceSq(src, uv) / 300.0;
		wavePower += sin((dist + iGlobalTime));
		
	}
	fragColor = vec4(
		0.5 + 0.5 * sin(wavePower),
		0.5 + 0.5 * cos(wavePower),
		0.5 + 0.5 * sin(iGlobalTime),
		1.0
	);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
