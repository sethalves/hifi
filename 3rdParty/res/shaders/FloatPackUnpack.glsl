// ------------------------------------------------------------------------
//  Float packing/unpacking
//  notations:
//  NUF : normalized unsigned float: f in [0, 1]
//  NUI16: normalized unsigned 16 bit integer: d in [0, 65535] 
//          packed into a vec2, where x,y in [0,1]
//  NUI24: normalized unsigned 24 bit integer: d in [0, 16777215]
//          packed into a vec3 where x,y,z in [0,1]
//  NUI32: normalized unsigned 32 bit integer: d in [0, 4294967295]
//          packed into a vec4 where x,y,z,w in [0,1]
// ------------------------------------------------------------------------
vec2 packNUFtoNUI16(float f)
{
    // f in [0,1]  --> d in integer [0, 65535]
    // d = x + y * 256 where x,y in integer [0, 255]
    // packedVal = [x,y]/255
	vec2 packedVal;
    
	float d = floor(f * 65535.0);
    packedVal.y = floor(d / 256.0);
    packedVal.x = d - packedVal.y * 256.0;
    
	return packedVal / 255.0;
}

float unpackNUI16toNUF(vec2 packedVal)
{
    // packedVal = [x,y] in [0,1]
    // uf = x * 255 + y * 255 * 256 in integer [0, 65535]
    // f = uf / 65535
	float f = dot(packedVal, vec2(255.0, 255.0 * 256.0));
	return f / 65534.9;
}


vec3 packNUFtoNUI24(float f)
{
    // f in [0,1]  --> d in integer [0, 16777215]
    // d = x + y * 256 + z * 65536 where x,y,z in integer [0, 255]
    // packedVal = [x,y,z]/255.0
	vec3 packedVal;
    
    float d = floor(f * 16777215.0);
	packedVal.z = floor(d / 65536.0);
	d = d - packedVal.z * 65536.0;
	packedVal.y = floor(d / 256.0);
	packedVal.x = d - packedVal.y * 256.0;
    
	return packedVal / 255.0;
}

float unpackNUI24toNUF(vec3 packedVal)
{
    // packedVal = [x,y,z] in [0,1]
    // uf = x * 255 + y * 255 * 256 + z * 255 * 65536 in integer [0, 16777215]
    // f = uf / 16777215
	float f = dot(packedVal, vec3(255.0, 255.0 * 256.0, 255.0 * 65536.0));
	return f / 16777214.9;
}


vec4 packNUFtoNUI32(float f)
{
    // f in [0,1]  --> d in integer [0, 4294967295]
    // d = x + y * 256 + z * 65536 + w * 16777216 where x,y,z,w in integer [0, 255]
    // packedVal = [x,y,z,w]/255.0
	vec4 packedVal;
    
	float d = floor(f * 4294967295.0);
	packedVal.w = floor(d / 16777216.0);
	d = d - packedVal.w * 16777216.0;
	packedVal.z = floor(d / 65536.0);
	d = d - packedVal.z * 65536.0;
	packedVal.y = floor(d / 256.0);
	packedVal.x = d - packedVal.y * 256.0;

	return packedVal / 255.0;
}

float unpackNUI32toNUF(vec4 packedVal)
{
    // packedVal = [x,y,z,w] in [0,1]
    // uf = x * 255 + y * 255 * 256 + z * 255 * 65536 + w * 255 * 16777216 in integer [0, 4294967295]
    // f = uf / 4294967295
	float f = dot(packedVal, vec4(255.0, 255.0 * 256.0, 255.0 * 65536.0, 255.0 * 16777216.0));
	return f / 4294967294.9;
}
