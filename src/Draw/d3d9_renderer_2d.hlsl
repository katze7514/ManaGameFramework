float4x4 matPerPixel; // ピクセル単位で描画するための行列

/////////////////////////////////////////
// ポリゴンをピクセル単位で描画する
float4 vs_pos(float3 pos : POSITION) : POSITION
{
	return mul(float4(pos,1.0f), matPerPixel);
}

/////////////////////////////////////////
// カラー付きポリゴンをピクセル単位で描画する
struct VS_COLOR_OUT
{
	float4 pos	 : POSITION;
	float4 color : COLOR0;
};


VS_COLOR_OUT vs_pos_color(float3 pos : POSITION, float4 color : COLOR0, float4 color1 : COLOR1)
{
	VS_COLOR_OUT OUT;
	OUT.pos = mul(float4(pos,1.0f), matPerPixel);
	OUT.color.rgb = color1.rgb;
	OUT.color.a = color.a;
	return OUT;
}

float4 ps_pos_color(float4 color : COLOR0) : COLOR0
{
	return color;
}

////////////////////////////////////////
// UV付き頂点をピクセル単位で描画する
texture tex;
sampler smp = sampler_state
{
	Texture = <tex>;

	MinFilter = Linear;
	MagFilter = Linear;
	MipFilter = NONE;
};

struct VS_UV_OUT
{
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
	float4 color : COLOR0;
};

VS_UV_OUT vs_pos_uv(float3 pos : POSITION, float2 uv : TEXCOORD0, float4 color : COLOR0)
{
	VS_UV_OUT OUT;
	
	OUT.pos = mul(float4(pos-float3(0.5f,0.5f,0.0f), 1.0f), matPerPixel); // テクセルピクセル変換
	OUT.uv = uv;
	OUT.color = color;
	return OUT;
}

float4 ps_pos_uv(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR0
{
	float4 OUT = tex2D(smp, uv);
	OUT.a = OUT.a * color.a; // αは乗算
	return OUT;
}

float4 ps_pos_uv_color1(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR0
{
	float4 OUT = tex2D(smp, uv);
	float a = OUT.a;
	//OUT.rgb = (1.0f-color.a)*OUT.rgb + color.a * color.rgb; // αブレンド
	OUT = (1.0f-color.a)*OUT + color.a * color; // αブレンド
	OUT.a = a;
	return OUT;
}

////////////////////////////////////////
// UV・カラー付き頂点をピクセル単位で描画する
struct VS_UV_COLOR_OUT
{
	float4 pos	  : POSITION;
	float2 uv     : TEXCOORD0;
	float4 color  : COLOR0;
	float4 color2 : COLOR1;
};

VS_UV_COLOR_OUT vs_pos_uv_color(float3 pos : POSITION, float2 uv : TEXCOORD0, float4 color : COLOR0, float4 color2 : COLOR1)
{
	VS_UV_COLOR_OUT OUT;
	
	OUT.pos = mul(float4(pos-float3(0.5f,0.5f,0.0f), 1.0f), matPerPixel); // テクセルピクセル変換
	OUT.uv = uv;
	OUT.color = color;
	OUT.color2 = color2;

	return OUT;
}

//////////////////////////////////////////
// color1の色をそのまま使う
float4 ps_pos_uv_color_thr(float2 uv : TEXCOORD0, float4 color : COLOR0, float4 color1 : COLOR1) : COLOR0
{
	float4 OUT = tex2D(smp, uv);
	OUT.rgb = color1.rgb;
	OUT.a = OUT.a * color.a * color1.a;
	return OUT;
}

//////////////////////////////////////////
// color1とテクスチャ色を通常α合成
float4 ps_pos_uv_color_blend(float2 uv : TEXCOORD0, float4 color : COLOR0, float4 color1 : COLOR1) : COLOR0
{
	float4 OUT = tex2D(smp, uv);
	OUT.rgb = (1.0f - color1.a) * OUT.rgb + color1.a * color1.rgb;
	OUT.a = OUT.a * color.a;
	return OUT;
}

//////////////////////////////////////////
// color1とテクスチャ色を乗算合成
float4 ps_pos_uv_color_mul(float2 uv : TEXCOORD0, float4 color : COLOR0, float4 color1 : COLOR1) : COLOR0
{
	float4 OUT = tex2D(smp, uv);
	OUT.rgb = OUT.rgb * color1.rgb * color1.a;
	OUT.a = OUT.a * color.a;
	return OUT;
}

//////////////////////////////////////////
// color1とテクスチャ色をスクリーン合成
float4 ps_pos_uv_color_screen(float2 uv : TEXCOORD0, float4 color : COLOR0, float4 color1 : COLOR1) : COLOR0
{
	const float3 base = {1.0f, 1.0f, 1.0f};
	float4 OUT = tex2D(smp, uv);
	float3 comp = color1.a * color1.rgb;
	OUT.rgb = OUT.rgb + comp - (OUT.rgb * comp);
	OUT.a = OUT.a * color.a;
	return OUT;
}

////////////////////////////////////////
// ディスタンスフィールド
float4 ps_df_color(float2 uv : TEXCOORD0, float4 color : COLOR0) : COLOR0
{
	float4 OUT = color;
	float4 df = tex2D(smp, uv);
	OUT.a = df.r>=0.5;
	/*if(OUT.a==0)
	{// 縁取り
		OUT.a = smoothstep(0.3, 0.5, df.r);
		OUT.r = 0;
		OUT.g = 0;
		OUT.b = 0;
	}*/
	return OUT;
}

////////////////////////////////////
// テクニック
technique base
{
	// ポリゴンだけを描画
	pass poly
	{
		VertexShader = compile vs_2_0 vs_pos();

		AlphaBlendEnable = false;
	}

	// ポリゴンだけを描画。カラーあり、αブレンドなし
	pass poly_color
	{
		VertexShader = compile vs_2_0 vs_pos_color();
		PixelShader  = compile ps_2_0 ps_pos_color();

		AlphaBlendEnable = false;
	}

	// ポリゴンだけを描画。カラーあり、αブレンドあり
	pass poly_color_blend
	{
		VertexShader = compile vs_2_0 vs_pos_color();
		PixelShader  = compile ps_2_0 ps_pos_color();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// UV付き頂点
	pass tex
	{
		VertexShader = compile vs_2_0 vs_pos_uv();
		PixelShader  = compile ps_2_0 ps_pos_uv();

		AlphaBlendEnable = false;
	}

	// UV付き頂点を描画
	// テクスチャのアルファでブレンド
	pass tex_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv();
		PixelShader  = compile ps_2_0 ps_pos_uv();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// テクスチャのアルファでブレンド
	pass tex_color1
	{
		VertexShader = compile vs_2_0 vs_pos_uv();
		PixelShader  = compile ps_2_0 ps_pos_uv_color1();

		AlphaBlendEnable = false;
	}

	// テクスチャにカラーをスルー合成。αブレンドなし
	pass tex_colorthr
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_thr();

		AlphaBlendEnable = false;
	}

	// テクスチャにカラーをスルー合成。αブレンドあり
	pass tex_colorthr_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_thr();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// テクスチャとカラーを通常α合成。αブレンドなし
	pass tex_colormul
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_blend();

		AlphaBlendEnable = false;
	}

	// テクスチャとカラーを通常α合成。αブレンドあり
	pass tex_colormul_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_blend();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// テクスチャとカラーを乗算合成。αブレンドなし
	pass tex_colormul
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_mul();

		AlphaBlendEnable = false;
	}

	// テクスチャとカラーを乗算合成。αブレンドあり
	pass tex_colormul_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_mul();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// テクスチャとカラーをスクリーン合成。αブレンドなし
	pass tex_colorscreen
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_screen();

		AlphaBlendEnable = false;
	}

	// テクスチャとカラーをスクリーン合成。αブレンドあり
	pass tex_colorscreen_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_pos_uv_color_screen();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}

	// DistanceFieldTexture 色はデフューズを使う
	pass df_color
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_df_color();

		AlphaBlendEnable = false;
	}

	pass df_color_blend
	{
		VertexShader = compile vs_2_0 vs_pos_uv_color();
		PixelShader  = compile ps_2_0 ps_df_color();

		AlphaBlendEnable = true;
		SrcBlend		 = SrcAlpha;
		DestBlend		 = InvSrcAlpha;
	}
}
