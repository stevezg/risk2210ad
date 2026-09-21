// Territory fill with a glowing neon border. uv.x is 0 on the polygon rim and 1 at its centre.
Shader "Risk2210/NeonTerritory"
{
    Properties
    {
        _FillColor ("Fill", Color) = (0.08, 0.11, 0.2, 0.85)
        _BorderColor ("Border", Color) = (0.4, 0.9, 1, 1)
        _BorderWidth ("Border Width", Range(0.02, 0.6)) = 0.18
        _Hover ("Hover", Range(0, 2)) = 0
        _Pulse ("Pulse", Range(0, 1)) = 0
    }
    SubShader
    {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" "IgnoreProjector"="True" }
        Blend SrcAlpha OneMinusSrcAlpha
        ZWrite Off
        Cull Off

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            fixed4 _FillColor, _BorderColor;
            float _BorderWidth, _Hover, _Pulse;

            struct appdata { float4 vertex : POSITION; float2 uv : TEXCOORD0; };
            struct v2f { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float2 world : TEXCOORD1; };

            v2f vert(appdata v)
            {
                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                o.uv = v.uv;
                o.world = mul(unity_ObjectToWorld, v.vertex).xy;
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float d = i.uv.x;                                   // 0 rim .. 1 centre
                float border = 1.0 - smoothstep(0.0, _BorderWidth, d);
                float glow = 1.0 - smoothstep(0.0, _BorderWidth * 2.5, d);
                float hatch = 0.5 + 0.5 * sin((i.world.x + i.world.y) * 40.0);
                float3 fill = _FillColor.rgb * (0.85 + 0.15 * hatch);
                float fillA = _FillColor.a * (0.7 + 0.3 * _Hover);
                float boost = 1.0 + _Hover * 1.2 + _Pulse * 0.6;
                float3 col = fill * fillA + _BorderColor.rgb * (border * 1.4 + glow * 0.5) * boost;
                float alpha = saturate(max(fillA, border + glow * 0.5));
                return fixed4(col, alpha);
            }
            ENDCG
        }
    }
}
