sampler2D TS0 : register(s0);
float4 gradient_min_color : register(c0);
float4 gradient_max_color : register(c1);
float4 background_color : register(c2);
float4 flash_color : register(c3);
float4 tint_color : register(c4);
float4 config : register(c5);

struct PS_INPUT {
   float4 Pos : SV_POSITION;
   float4 D0 : COLOR0;
   float4 D1 : COLOR1;
   float4 T0 : TEXCOORD0;
};

float4 main(PS_INPUT i) : COLOR0 {
    float4 Tex0 = i.T0;

    float4 t0 = tex2D(TS0, Tex0.xy);

    if(config.x > 0.5) {
        float4 inv_tex_point = t0;
        t0 = float4(inv_tex_point.aaa, inv_tex_point.r);
    }

    float4 r0, r1;

    // clip by non-interpolated alpha
    clip(t0.a - config.y);

    // S0
	r1.a = (gradient_max_color.a) * (t0.b);

	r0.rgb = (gradient_min_color.aaa - t0.rgb) * 4;
	r0.a = saturate((r1.a + r1.a) * 4);

    // S1
	r1.rgb = gradient_min_color.rgb + (r0.a) * (gradient_max_color.rgb - gradient_min_color.rgb);
	r0.a = 2 * (-r0.b + 0.5);
    r0.rgb = r1.rgb;

    // S2
    float3 c5 = lerp(flash_color.rgb, -flash_color.rgb, config.a);
	r0.rgb = saturate(r0.a * c5.rgb) + r0.rgb;
	r0.a = t0.b - flash_color.a + 0.5;

    // S3
	r0.rgb = lerp(r0.rgb, background_color.rgb, step(0.5, r0.a));
	r0.a = lerp(tint_color.a, background_color.a, step(0.5, r0.a));

    // Final combiner
	r0.rgb = r0.rgb * t0.a;

    return r0;
};
