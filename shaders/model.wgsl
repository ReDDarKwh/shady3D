
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) normal: vec3f, 
	@location(1) color: vec3f,
};

struct FrameUniforms {
    mvp: mat4x4f,
	m: mat4x4f,
    time: f32,
};

@group(0) @binding(0) var<uniform> frame: FrameUniforms;
@group(1) @binding(0) var<uniform> iRes: vec2u;

@vertex
fn vs_main(
    in: VertexInput
) -> VertexOutput {

    var out: VertexOutput;
	out.position = frame.mvp * vec4f(in.position, 1.0);
    out.normal = (frame.m * vec4f(in.normal, 0)).xyz;
	out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

	let normal = normalize(in.normal);

	let lightColor1 = vec3f(1.0, 0.9, 0.6);
	let lightColor2 = vec3f(0.6, 0.9, 1.0);
	let lightDirection1 = vec3f(0.5, -0.9, 0.1);
	let lightDirection2 = vec3f(0.2, 0.4, 0.3);
	let shading1 = max(0.0, dot(lightDirection1, normal));
	let shading2 = max(0.0, dot(lightDirection2, normal));
	let shading = shading1 * lightColor1 + shading2 * lightColor2;
	let color = shading;

	// Gamma-correction
	let corrected_color = pow(color, vec3f(2.2));
	return vec4f(normal, 1);
}

