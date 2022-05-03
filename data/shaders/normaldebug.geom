#version 450

layout (triangles) in;
layout (line_strip, max_vertices = 8) out;

layout(set = 0, binding = 1) uniform UBO {
	mat4 view;
	mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts{
	mat4 model;
} primitive;

layout (location = 0) in vec4 inColor[];
layout (location = 1) in vec2 inTexCoord[];
layout (location = 2) in vec3 inNormal[];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;

void main(void){	
	float normalLength = 0.3f;
	vec3 face_pos = vec3(0.0f, 0.0f, 0.0f);

	for(int i=0; i<gl_in.length(); i++){
		vec3 pos = gl_in[i].gl_Position.xyz;
		vec3 normal = inNormal[i].xyz;

		face_pos += pos;

		gl_Position = ubo.proj * ubo.view * primitive.model * vec4(pos, 1.0f);
		outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		EmitVertex();

		gl_Position = ubo.proj * ubo.view * primitive.model * vec4(pos + normal * normalLength, 1.0f);
		outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		EmitVertex();

		EndPrimitive();
	}

	face_pos /= 3.0f;
	vec3 face_normal = normalize(cross((gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz),
									   (gl_in[0].gl_Position.xyz - gl_in[2].gl_Position.xyz)));

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(face_pos, 1.0f);
	outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	EmitVertex();

	gl_Position = ubo.proj * ubo.view * primitive.model * vec4(face_pos + face_normal * normalLength, 1.0f);
	outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	EmitVertex();

	EndPrimitive();
}