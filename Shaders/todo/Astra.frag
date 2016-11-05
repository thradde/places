
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

#define iResolution resolution
#define iMouse mouse

#define EPSILON .001
const float PI=3.14159265;
const int MAX_ITER = 120;
vec3 lightDir    =normalize(vec3(1., 1., -1.));
vec4 col1 = vec4(0.155, 0., 1., 1.);
vec4 col2 = vec4(0., 0.86, 1., 1.);

struct mat
{
  float typeMat;        
     
};
mat materialMy = mat(0.0);
vec3 colGlow = vec3(0.);
float alfa = 0.3;
//-----------------------------
vec3 getNormal(in vec3 p);
float renderFunction(in vec3 pos);
float render(in vec3 posOnRay, in vec3 rayDir);
vec4 getColorPixel(inout vec3 ro, vec3 rd, inout vec3 normal, float dist, float typeColor);
//-----------------------------
//---------------------------------------------
vec3 rotationCoord(vec3 n)
{
 vec3 result;
 //--------------------------------------------
   float t = time * 0.1;
   vec2 sc = vec2(sin(t), cos(t));
   mat3 rotate;
  
    mat3 rotate_x = mat3(  1.0,  0.0,  0.0,
                          0.0, sc.y,-sc.x,
                          0.0, sc.x, sc.y);
   mat3 rotate_y = mat3( sc.y,  0.0, -sc.x,
                         0.0,   1.0,  0.0,
                         sc.x,  0.0,  sc.y);
   mat3 rotate_z = mat3( sc.y, sc.x,  0.0,
                        -sc.x, sc.y,  0.0,
                         0.0,  0.0,   1.0);
   rotate = rotate_z * rotate_y * rotate_z;                
  result = n * rotate;
  return result;
}
//----------------------------------------------------
//------------------------------------------
vec2 rot(vec2 p,float r){
  vec2 ret;
  ret.x=p.x*cos(r)-p.y*sin(r);
  ret.y=p.x*sin(r)+p.y*cos(r);
  return ret;
}
//------------------------------------------
vec2 rotsim(vec2 p,float s)
{
  vec2 ret=p;
  ret=rot(p,-PI/(s*2.0));
  ret=rot(p,floor(atan(ret.x,ret.y)/PI*s)*(PI/s));
  return ret;
}
//------------------------------------------

//?? ?????? https://www.shadertoy.com/view/ltXGRj
float rays(in vec3 p, in vec2 size, in float r)
{
   float rad=length(p);
   return (length(p.xz)- size.x + size.y * (rad - r)); 
}
float astra(in vec3 p, in vec2 kol, in vec2 size, in float r)
{
   p.xy+=sin(p.yx*(6. + 2.*sin(time*0.1))-time*2.0)* 0.1;
   p.xy = rotsim(p.xy, kol.x);
   p.zy = rotsim(p.zy, kol.y);
   return rays(p, size, r);
}
//--------------------------------------------------
float smin( float a, float b, float k ) 
{
   float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
   return mix( b, a, h ) - k*h*(1.0-h);
}
//------------------------------------------
float distMat(inout float curDist, float dist, in float typeMat)
{
   float res = curDist;
   if (dist < curDist) 
   {
      materialMy.typeMat     = typeMat;
      res                    = dist;
     return res;
   }
   return curDist;
}

//--------------------------------------------------
float myObject(in vec3 p)
{
   float d =  1.0;
   materialMy.typeMat = 0.0;
   vec3 pos = p;
   float r = 0.;

   pos = rotationCoord(pos);
   r = 5. * sin(time  * 0.2)*0.5+0.5;// sin(time )
   d =  distMat(d,   astra(pos, vec2(10., 10.), vec2(0.16, 0.04), r),  2.);   

   return d; 
}
//-------------------------------------------------
// ????? ???????
float renderFunction(in vec3 pos)
{
    return  myObject(pos);    
}
//------------------------------------------------- 
vec3 getNormal(in vec3 p)
{

const float precis = 0.001;
    vec3  eps = vec3(precis,0.0,0.0);
    vec3 nor;
    nor.x = renderFunction(p+eps.xyy) - renderFunction(p-eps.xyy);
    nor.y = renderFunction(p+eps.yxy) - renderFunction(p-eps.yxy);
    nor.z = renderFunction(p+eps.yyx) - renderFunction(p-eps.yyx);
    return normalize(nor);

}
//-------------------------------------------------
vec3 getLighting(in vec3 ro, in vec3 rd ,in vec3 norm, in vec3 lightDir, in vec4 color)
{
    vec3 col = vec3(0.);

    vec3 ref = reflect( rd, norm );
    float rim = 0.04+0.96*pow( clamp( 1.0 + dot(norm,rd), 0.0, 1.0 ), 4.0 );
    col += mix( vec3(0.05,0.02,0.0), 1.2*vec3(0.8,0.9,1.0), 0.5 + 0.5*norm.y );
    col *= 1.0 + 1.5*vec3(0.7,0.5,0.3)*pow( clamp( 1.0 + dot(norm,rd), 0.0, 1.0 ), 2.0 );
    col += 5. *clamp(0.3 + 2. * norm.y, 0.0,1.0);//*(rim);
    col *= color.rgb;
    col = pow( col, vec3(0.5) );
  
//   vec2 q =  fragCoord.xy / iResolution.xy ;    
 //  col *= -0.16 + 0.68 * pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.15 );  
    return col ;    

}

//----------------------------------------------------------------------
vec4 getColorPixel(inout vec3 ro, vec3 rd, inout vec3 normal, float dist, float typeColor)
{

  vec4 color = vec4(1.);
  vec3 hitPos = ro + rd * dist;
  normal = normalize(getNormal(hitPos));  
//----------------------------------
   if (materialMy.typeMat == 0.0) 
   {
       vec3 col = mix( vec3(1., 0., 0.42 ), vec3(0., 0.3, 1), 0.5  + 0.5 * rd.y );
       color.rgb =  col ;
   } 
   else if (materialMy.typeMat == 1.0)    
        color.rgb = texture2D( iChannel1, 0.15 * hitPos.xz ).xyz;
   else if (materialMy.typeMat == 2.)  
        color.rgb = colGlow;
        
   if(materialMy.typeMat !=0. )
      color.rgb =  getLighting(hitPos, rd, normal, lightDir, color);
   
    ro = hitPos;
    
    color.rgb = mix(color.rgb, colGlow, 0.5);
  return color;
}

//-------------------------------------------------
//https://www.shadertoy.com/view/MtlGzX
vec4 colorMix(in float dist, in vec3 col1, in vec3 col2)
{
    float opacity = smoothstep(0.3, .0, dist);
//    float opacity = smoothstep(0.4, .0,sqrt( dist));
    opacity *= opacity;
    return vec4(mix(vec3(col1), vec3(col2), opacity), opacity);
}

float render(in vec3 posOnRay, in vec3 rayDir)
{ 
  float t = 0.0;
  float maxDist = 40.;
  float d = 0.1;  
  vec3 scenecol = vec3(0.);
  float opacity = 0.;
  vec3 color1 = vec3(0.), color2 = color1;
  float k =sin(time  * 0.3)*0.5+0.5;      
  for(int i=0; i<MAX_ITER; ++i)
  {
     if ( opacity > .95 ) break;

    if (abs(d) <EPSILON || t > maxDist) 
         break;
    t += d;
    vec3 ro = posOnRay + t*rayDir;
    d = renderFunction(ro);     
    
    //----------------------
    if(materialMy.typeMat == 2.)
    {
        float  clarity = smoothstep(0.5, 0., sqrt( d ));
        vec4 currCol = vec4(mix(vec3(col1), vec3(col2),  clarity *  clarity),  clarity);
        color1 += (1. -  clarity) * currCol.rgb * currCol.a ;
       
        vec4 currCol1 = colorMix(d, vec3(1., 0.38, 0.48), vec3(1., 0.66, 0.));
        color2 +=  (1. - opacity) * currCol1.rgb * currCol1.a;
        opacity +=  (1. - opacity) * currCol.a;          
        colGlow = mix(color1,color2,k);
    }
    
  }
   return t;
}
//------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos     =  fragCoord.xy / iResolution.xy * 2. - 1.;
    pos.x *= iResolution.x / iResolution.y;  

    float t = time* 0.1;
    vec3 camP = vec3(0., 0, 10.);
    vec3 camUp = vec3(0. , 1., 0.);
    vec3 camDir = normalize(-camP);
    vec3 u = normalize(cross(camUp,camDir));
    vec3 v = cross(camDir,u);
    vec3 rayDir = normalize(camDir * 2. + pos.x * u + pos.y * v);  

   vec4 color    = vec4(1.0);
    vec3 normal   = vec3(1.0);
    vec3 posOnRay = camP; 
    float path = 0.;
  //--------------------------- 
     path =  render(posOnRay, rayDir);  
     color = getColorPixel(posOnRay, rayDir, normal, path, materialMy.typeMat); 
    // ----------------------------------------------------------------------
    // POST PROCESSING
    // Gamma correct
     color.rgb = pow(color.rgb , vec3(0.45));
    // Contrast adjust - cute trick learned from iq
     color.rgb  = mix( color.rgb , vec3(dot(color.rgb ,vec3(0.333))), -0.6 );
     color.a = 1.;
     fragColor =  color;
}

void main(void)
{
	mainImage(gl_FragColor, gl_FragCoord.xy);
}
