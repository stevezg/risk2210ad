// Pass 0: bright pass. Pass 1: separable blur (direction in _Dir). Pass 2: composite (+ optional scanlines).
Shader "Risk2210/Bloom"
{
    Properties { _MainTex ("Texture", 2D) = "white" {} }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always
        CGINCLUDE
        #include "UnityCG.cginc"
        sampler2D _MainTex, _BloomTex;
        float4 _Dir;
        float _Threshold, _Intensity, _Scanlines;
        struct v2f { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
        v2f vert(appdata_img v) { v2f o; o.pos = UnityObjectToClipPos(v.vertex); o.uv = v.texcoord; return o; }
        ENDCG

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            fixed4 frag(v2f i) : SV_Target
            {
                fixed4 c = tex2D(_MainTex, i.uv);
                float lum = dot(c.rgb, float3(0.299, 0.587, 0.114));
                return fixed4(c.rgb * saturate((lum - _Threshold) * 3.0), 1);
            }
            ENDCG
        }
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            fixed4 frag(v2f i) : SV_Target
            {
                float w[5] = { 0.227, 0.195, 0.122, 0.054, 0.016 };
                float3 sum = tex2D(_MainTex, i.uv).rgb * w[0];
                for (int k = 1; k < 5; k++)
                {
                    sum += tex2D(_MainTex, i.uv + _Dir.xy * k * 1.6).rgb * w[k];
                    sum += tex2D(_MainTex, i.uv - _Dir.xy * k * 1.6).rgb * w[k];
                }
                return fixed4(sum, 1);
            }
            ENDCG
        }
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            fixed4 frag(v2f i) : SV_Target
            {
                fixed4 c = tex2D(_MainTex, i.uv);
                float3 bloom = tex2D(_BloomTex, i.uv).rgb * _Intensity;
                float3 col = c.rgb + bloom;
                float scan = 0.5 + 0.5 * sin(i.uv.y * _ScreenParams.y * 1.5);
                col *= 1.0 - 0.15 * _Scanlines * scan;
                return fixed4(col, 1);
            }
            ENDCG
        }
    }
}
