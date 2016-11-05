// attempt at spark creation - based on Blobs by @paulofalcao

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iResolution resolution
#define iMouse mouse

float makePoint(float x,float y,float sparkDur,float fy,float sparkDist,float xShakeWidth,float t,float offset){
   float xx=x+sin(t*sparkDur*3.0)*xShakeWidth;
    //float xx=x+sin(t*fx)*sx;
    //changed below from fy/sy -- this just creates different x/y orbit bases
   float yy=y+cos(t*sparkDur)*0.4*sparkDist;
    //float yy=y+cos(t*fx)*sx;
    float overTime = 1.0/(sqrt(xx*xx+yy*yy));
   //return 1.0/(sqrt(xx*xx+yy*yy));//+abs(cos(t*fx)));//+offset
    
    //multiplying by the sin makes light fade in then out on upward cos swing
    return overTime*sin(t*sparkDur);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

   vec2 p=(fragCoord.xy/iResolution.x)*2.0-vec2(1.0,iResolution.y/iResolution.x);

   p=p*2.0;
   
   float x=p.x;
   float y=p.y;

   float a=
       makePoint(x,y,3.0,2.9,0.8,0.15,time,0.1);
   a=a+makePoint(x,y,1.9,2.0,0.8,0.2,time,0.4);
   a=a+makePoint(x,y,0.8,0.7,0.4,0.17,time,0.7);
   a=a+makePoint(x,y,2.3,0.1,0.6,0.3,time,2.5);
   a=a+makePoint(x,y,0.8,1.7,0.5,0.24,time,1.6);
   a=a+makePoint(x,y,0.3,1.0,0.4,0.33,time,0.8);
   a=a+makePoint(x,y,1.4,1.7,0.4,0.08,time,1.3);
   a=a+makePoint(x,y,1.3,2.1,0.6,0.14,time,2.3);
   a=a+makePoint(x,y,1.8,1.7,0.5,0.14,time,2.8);   
   
   /*float b=
       makePoint(x,y,1.2,1.9,0.8,0.3,time,0.6);
   b=b+makePoint(x,y,0.7,2.7,0.4,0.4,time);
   b=b+makePoint(x,y,1.4,0.6,0.4,0.5,time);
   b=b+makePoint(x,y,2.6,0.4,0.6,0.3,time);
   b=b+makePoint(x,y,0.7,1.4,0.5,0.4,time);
   b=b+makePoint(x,y,0.7,1.7,0.4,0.4,time);
   b=b+makePoint(x,y,0.8,0.5,0.4,0.5,time);
   b=b+makePoint(x,y,1.4,0.9,0.6,0.3,time);
   b=b+makePoint(x,y,0.7,1.3,0.5,0.4,time);*/

   /*float c=
       makePoint(x,y,3.7,0.3,0.8,0.3,time,0.8);
   c=c+makePoint(x,y,1.9,1.3,0.4,0.4,time);
   c=c+makePoint(x,y,0.8,0.9,0.4,0.5,time);
   c=c+makePoint(x,y,1.2,1.7,0.6,0.3,time);
   c=c+makePoint(x,y,0.3,0.6,0.5,0.4,time);
   c=c+makePoint(x,y,0.3,0.3,0.4,0.4,time);
   c=c+makePoint(x,y,1.4,0.8,0.4,0.5,time);
   c=c+makePoint(x,y,0.2,0.6,0.6,0.3,time);
   c=c+makePoint(x,y,1.3,0.5,0.5,0.4,time);*/
   
   vec3 d=vec3(a,a-y*32.0,a-y*50.0)/32.0;
   // vec3 d=vec3(a,b,c)/32.0;
   
   fragColor = vec4(d.x,d.y,d.z,1.0);
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
