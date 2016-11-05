// Procedural Tiles // Based on http://www.iquilezles.org/www/articles/smoothvoronoi/smoothvoronoi.htm

#ifdef GL_ES
precision mediump float;
#endif

const vec2 mouse = vec2(50,50);
const vec2 resolution = vec2(100,100);
const float time = 0.0;
// Expensive Noise

vec4 textureRND2D(vec2 uv){
	uv = floor(fract(uv)*1e3);
	float v = uv.x+uv.y*1e3;
	return fract(1e5*sin(vec4(v*1e-2, (v+200.)*1e-2, (v+1e3)*1e-2, (v+1e3+1.)*1e-2)));
}

float noise(vec2 p) {
	vec2 f = fract(p*1e3);
	vec4 r = textureRND2D(p);
	f = f*f*(3.0-2.0*f);
	return (mix(mix(r.x, r.y, f.x), mix(r.z, r.w, f.x), f.y));	
}

vec2 random2f( vec2 seed ) {
	#define rnd_seed 1.337
	float rnd1 = mod(noise(seed*rnd_seed), 1.0);
	float rnd2 = mod(rnd1*2.0,1.0);
	
	return vec2(rnd1, rnd2);
}

// Cheap Noise

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 rand2(vec2 co){
	float rnd1 = rand(co);
	float rnd2 = rand(co*rnd1);
	return vec2(rnd1,rnd2);
}


// Methods

vec3 tile_color = vec3(0.0);

#define tile_height 0.35
float voronoi( in vec2 x ) {
	vec2 p = floor( x );
	vec2 f = fract( x );
	
	vec3 res = vec3(1.0);
	
	for( int j=-1; j<=1; j++ ) for( int i=-1; i<=1; i++ ) {
		vec2 b = vec2( i, j );
		//vec2 r = vec2( b ) + rand2( p + b ) - f; // cheap
		vec2 r = vec2( b ) + random2f( p + b ) - f; // expensive but has some nicer properties for morphing
		float d = dot( r , r );
		
		if ( d < res.x ) {
			res.xyz = vec3(d,res.xy);
			if (rand(p+b) < 0.5) tile_color = vec3(.77,.87,.9);
			else tile_color = vec3(0.9,0.9,0.9);
		} else if (d < res.y) {
			res.yz = vec2(d,res.y);
		}
    	}
	
	return clamp(sqrt(res.y) - sqrt(res.x),0.0,tile_height);
}

vec3 normal(vec2 p) {
	float d = 0.001;
	float d2 = 0.01; // Smoothing parameter for normal
	vec3 dx = vec3(d2, 0.0, voronoi(p + vec2(d2, 0.0))) - vec3(-d, 0.0, voronoi(p + vec2(-d, 0.0)));
	vec3 dy = vec3(0.0, d2, voronoi(p + vec2(0.0, d2))) - vec3(0.0, -d, voronoi(p + vec2(0.0, -d)));
	return normalize(cross(dx,dy));
}

void main( void ) {

	vec2 p = vec2(sin(time),-cos(time)) + (3.*gl_FragCoord.xy)/resolution.y - mouse * vec2(40.0,20.0);
	
	float color = voronoi(p);
	
	float light_intensity = 0.75/tile_height;
	vec3 light = normalize(vec3(1.0,0.1,1.0)) * light_intensity;
	
	float shade = dot(light,normal(p))+0.5;
	gl_FragColor = vec4(vec3(shade*color) * tile_color, 1.0);
}
