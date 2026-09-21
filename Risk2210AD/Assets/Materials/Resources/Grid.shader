// World-space tactical grid with a slow energy sweep and vignette, drawn on a quad behind the board.
Shader "Risk2210/Grid"
{
    Properties { _Area ("Area (x, y, w, h)", Vector) = (0, 0, 1, 1) }
    SubShader
    {
        Tags { "Queue"="Background" "RenderType"="Opaque" }
        ZWrite Off
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            float4 _Area;
            struct v2f { float4 pos : SV_POSITION; float2 world : TEXCOORD0; float2 uv : TEXCOORD1; };
            v2f vert(float4 v : POSITION, float2 uv : TEXCOORD0)
            {
                v2f o; o.pos = UnityObjectToClipPos(v); o.world = mul(unity_ObjectToWorld, v).xy; o.uv = uv; return o;
            }
            float gridLine(float2 p, float spacing, float width)
            {
                float2 g = abs(frac(p / spacing - 0.5) - 0.5) * spacing;
                return 1.0 - smoothstep(0.0, width, min(g.x, g.y));
            }
            fixed4 frag(v2f i) : SV_Target
            {
                float3 base = float3(0.03, 0.04, 0.075);
                float minor = gridLine(i.world, 0.5, 0.018) * 0.28;
                float major = gridLine(i.world, 2.0, 0.03) * 0.55;
                float3 gridCol = float3(0.10, 0.55, 0.80);
                float3 col = base + gridCol * (minor + major);
                float sweep = exp(-pow((i.world.x + i.world.y * 0.4 - fmod(_Time.y * 1.8, 48.0) + 14.0) / 1.2, 2.0));
                col += gridCol * sweep * (minor + major) * 1.0;
                float2 c = i.uv - 0.5;
                col *= 1.0 - dot(c, c) * 0.7;
                return fixed4(col, 1);
            }
            ENDCG
        }
    }
}
