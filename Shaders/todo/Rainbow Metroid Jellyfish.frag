//
// Rainbow Metroid Jellyfish
//
// @christinacoffin
// 2015-05-08 - rough sketch , maybe add more depth and bioluminesence inside it later!
//
//
// used the blob glowcode here as a starting base: https://www.shadertoy.com/view/MtS3Wh
//
float glowstar(float x,float y,float sparkDur,float fy,float sparkDist,float xShakeWidth,float t,float offset)
{
    x = abs(x)+(t*0.25);
    y += abs(x*y*t*0.1);

    sparkDist *= 5.0;

    float xx=x+sin(t*sparkDur*3.0)*xShakeWidth;
    float yy=y+cos(t*sparkDur)*0.4*sparkDist;

    yy = abs(yy);
    xx += (xx*yy)-abs(yy);

    float speed = 1.0;
    float starFalloff;
    float starLight = 0.0;
    
   	starFalloff = ((xx*xx)*(y*xx*cos(xx*x*y)+y+x*2.0))+(yy*yy);// 
    
    float star_0 = starFalloff;
    
    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy); //octstar
   
    float star_1 = starFalloff;
    
    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy/t)*abs(y*y*y*y);
    
    float star_2 = starFalloff;
    
	starLight = sqrt(xx*xx+yy*yy);

    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy)*t*1.0;

    float overTime = 0.9/(starFalloff*speed);

    float final = abs(overTime*sin(t*sparkDur));
    
    final = pow(final,0.235);// smaller pow under 1.0, soft and dim stars
    return final;//adding abs removes the 'darklights'
}





float makePoint(float x,float y,float sparkDur,float fy,float sparkDist,float xShakeWidth,float t,float offset)
{
    x = abs(x)+(t*0.25);
    y += abs(x*y*t*0.1);
    
   sparkDist *= 5.0;//10.1;

   float xx=x+sin(t*sparkDur*3.0)*xShakeWidth;
   float yy=y+cos(t*sparkDur)*0.4*sparkDist;

    yy = abs(yy);
     xx += (xx*yy)-abs(yy);
    
    float speed = 1.0;
    float starFalloff;
    float starLight = 0.0;
    
   	starFalloff = ((xx*xx)*(y*xx*cos(xx*x*y)+y+x*2.0))+(yy*yy);// 

    float star_0 = starFalloff;
    
    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy); //octstar
   
    float star_1 = starFalloff;
    
    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy/t)*abs(y*y*y*y);
    
    float star_2 = starFalloff;
    
	starLight = sqrt(xx*xx+yy*yy);

    starFalloff -= sqrt(xx*xx*xx*xx+yy*yy*yy*yy)*t*1.0;
  
	//starFalloff /= 0.1*abs(cos(tan(xx*x*x*t*y*y*xx))/t*10.1);//add more lines inside, but additive
 
     starFalloff *= starFalloff/0.52;//darken mandible tentacles

    
    float overTime = 0.61799/(starFalloff*speed);

    float final = abs(overTime*sin(t*sparkDur));
    
    final = pow(final,0.235);
	starLight -= pow(starLight, 0.412171);
    final = starLight - abs(final);
    
    final -= fract(star_2*0.01/star_0);

    return final;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

    float periodTime = 1.9;//limit the animation cycle to the period that looks the most interesting and loop it.
    float time=(sin(iGlobalTime*0.1)*sin(iGlobalTime*0.1))*periodTime;
    time = abs(time)+0.9;
    
   vec2 p=(fragCoord.xy/iResolution.x)*2.0-vec2(1.0,iResolution.y/iResolution.x);

   p=p*2.0;
   
   p = p * 1.3;//scale down another bit to zoom in
   float x=0.0151+ p.x+sin(time*0.70);
   float y=(p.y)+0.5+sin(time)-1.0;

 	//skew and offset a bit in screenspace to frame the part i like
   y -= abs(x+y)*0.1;
   y -= 0.2;

    
   float a=
       makePoint(x,y,3.0,2.9,0.8,1.15,time,0.1);
   a=a+makePoint(x,y,1.9,2.0,0.8,0.2,time,0.4);
   a=a+makePoint(x,y,0.8,0.7,0.4,0.17,time,0.7);
   a=a+makePoint(x,y,2.3,0.1,0.6,0.3,time,2.5);
   a=a+makePoint(x,y,0.8,1.7,0.5,0.24,time,1.6);
   a=a+makePoint(x,y,0.3,1.0,0.4,0.33,time,0.8);
   a=a+makePoint(x,y,1.4,1.7,0.4,0.08,time,1.3);
   a=a+makePoint(x,y,1.3,2.1,0.6,0.14,time,2.3);
   a=a+makePoint(x,y,1.8,1.7,0.5,0.14,time,2.8);   
   
    //more layers
   float b=
       makePoint(x,y,1.2,1.9,0.8,1.3,time,0.6);
   b=b+makePoint(x,y,0.7,2.7,0.4,0.41,time,0.16);
    
   b=b+makePoint(x,y,1.4,0.6,0.4,0.52,time,0.26);
   b=b+makePoint(x,y,2.6,0.4,0.6,0.33,time,0.36);
   b=b+makePoint(x,y,0.7,1.4,0.5,0.44,time,0.46);
   b=b+makePoint(x,y,0.7,1.7,0.4,0.45,time,0.56);
   b=b+makePoint(x,y,0.8,0.5,0.4,0.56,time,0.66);
   b=b+makePoint(x,y,1.4,0.9,0.6,0.37,time,0.76);
   b=b+makePoint(x,y,0.7,1.3,0.5,0.48,time,0.86);
  
   vec3 d=vec3(a,a+a-y*32.0,a-y*50.0)/32.0;
    
   d += vec3(b,b+b-y*32.0,b-y*50.0)/32.0; 
    //add in electric glow
   d.x += max( d.y, glowstar(x,y,3.0,2.9,0.8,1.15,time*time,0.1)*0.7 );
    
	fragColor = vec4(d.x,d.y,d.z,1.0);
    //flip colors
    fragColor.xyz = fragColor.zyx;
}
