
/*
 * d39d_2d_renderクラスの初期化や終了に関わるメソッド実装
 */
#include "../mana_common.h"

#include "d3d9_renderer_2d.h"

namespace mana{
namespace draw{

///////////////////////////////////////////
// 初期化系
///////////////////////////////////////////
d3d9_renderer_2d::d3d9_renderer_2d():pVertexDecl_(nullptr), pPosVertex_(nullptr), pUvVertex_(nullptr), pColorVertex_(nullptr), pColor2Vertex_(nullptr), pIndexBuffer_(nullptr),
									 pEffect_(nullptr), hTechnique_(NULL),hMatPerPixel_(NULL),hTex_(NULL),
									 pRenderPosVertex_(nullptr), pRenderUvVertex_(nullptr), pRenderTexture_(nullptr), pRenderTarget_(nullptr),pBackBuf_(nullptr),
									 nRenderTaregetColor_(D3DCOLOR_ARGB(255,0,0,0)),nRenderTaregetVertexColor_(D3DCOLOR_ARGB(0,255,255,255)),
									 nRenderWidth_(0),nRenderHeight_(0),fMaxZ_(DEFAULT_MAX_Z),
									 nMaxDrawSpriteNum_(DEFAULT_MAX_SPRITE_NUM),nMaxDrawCallPrimitiveNum_(DEFAULT_MAX_SPRITE_NUM),
									 bScreenShot_(false),nRenderCounter_(0),eDlState_(DEVLOST_NO)
{
	D3DXMatrixIdentity(&matPerPixel_);
	D3DXMatrixIdentity(&matRenderPerPixel_);

	bResetDevice_.store(false, std::memory_order_release);

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	nMaxDrawSpriteCounter_=nMaxDrawCallSpriteCounter_=nMaxDrawCallCounter_=nMaxTextureVram_=0;
#endif
}

d3d9_renderer_2d::~d3d9_renderer_2d()
{ 
#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	// 各種カウンター
	logger::infoln("[d3d9_render_2d]1レンダーで使われる最大スプライト数            : " + to_str(nMaxDrawSpriteCounter_));
	logger::infoln("[d3d9_render_2d]1レンダーの1DrawCallで使われる最大スプライト数 : " + to_str(nMaxDrawCallSpriteCounter_));
	logger::infoln("[d3d9_render_2d]1レンダーで呼ばれるDrawCall最大数              : " + to_str(nMaxDrawCallCounter_));
	logger::infoln("[d3d9_render_2d]レンダーで使われたテクスチャピーク容量         : " + to_str(nMaxTextureVram_));
#endif

	fin();
}

// 終了処理
void d3d9_renderer_2d::fin()
{
	safe_release(pRenderTarget_);
	safe_release(pRenderTexture_);
	safe_release(pRenderPosVertex_);
	safe_release(pRenderUvVertex_);
	safe_release(pBackBuf_);

	hTechnique_ = hMatPerPixel_ = hTex_ = NULL;
	safe_release(pEffect_);

	safe_release(pPosVertex_);
	safe_release(pUvVertex_);
	safe_release(pColorVertex_);
	safe_release(pColor2Vertex_);

	safe_release(pIndexBuffer_);
	safe_release(pVertexDecl_);

	texManager_.fin();
}

bool d3d9_renderer_2d::init(d3d9_device_init& deviceParam, renderer_2d_init& param)
{
	fin();

	pDriver_ = make_shared<d3d9_driver>();

	// D3D9作成
	if(!pDriver_->create_driver()) return false;

	// render_2dが要求する形に上書きする
	deviceParam.nDepth_		= 24;
	deviceParam.bStencil_	= true;

	// D3D9デバイス作成
	if(!pDriver_->create_device(deviceParam)) return false;

	if(param.nRenderWidth_==0)
		param.nRenderWidth_ = pDriver_->wnd_width();

	if(param.nRenderHeight_==0)
		param.nRenderHeight_ = pDriver_->wnd_height();

	return init_inner(param);
}

bool d3d9_renderer_2d::init(const d3d9_driver_sptr& pDriver, renderer_2d_init& param)
{
	fin();

	pDriver_ = pDriver;

	if(param.nRenderWidth_==0)
		param.nRenderWidth_ = pDriver_->wnd_width();

	if(param.nRenderHeight_==0)
		param.nRenderHeight_ = pDriver_->wnd_height();

	return init_inner(param);
}

bool d3d9_renderer_2d::init_inner(renderer_2d_init& param)
{
	nRenderWidth_	= param.nRenderWidth_;
	nRenderHeight_	= param.nRenderHeight_;
		
	if(nRenderWidth_==0 || nRenderWidth_>pDriver_->wnd_width())
		nRenderWidth_ = pDriver_->wnd_width();

	if(nRenderHeight_==0 || nRenderHeight_>pDriver_->wnd_height())
		nRenderHeight_ = pDriver_->wnd_height();

	if(param.fMaxZ_>0)
		fMaxZ_	= param.fMaxZ_;

	nMaxDrawSpriteNum_			= param.nMaxDrawSpriteNum_;
	nMaxDrawCallPrimitiveNum_	= param.nMaxDrawCallPrimitiveNum_;
	
	if(!check_caps()					// 必要な能力を持つかチェック
	|| !init_vertex()					// 頂点関係初期化
	|| !init_effect(param.sShaderPath_)	// シェーダー関係初期化
	|| !init_render_target()			// レンダーターゲット関係初期化
	)
	{// 一つでも転けたらfalse
		fin();
		return false;
	}

	// テクスチャ関係初期化
	texManager_.init(pDriver_, param.nReserveTextureNum_, param.nTextureMaxVRAM_);
	// テキスト関係初期化
	renderText_.init(param.nReserveFontNum_);
	// 最後にレンダーの初期化
	init_render();

	// プロパティ初期化
	nRenderCounter_	= 0;
	eDlState_		= DEVLOST_NO;

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	nMaxDrawSpriteCounter_		= 0;
	nMaxDrawCallSpriteCounter_	= 0;
	nMaxDrawCallCounter_		= 0;
	nMaxTextureVram_			= 0;
#endif

	logger::infoln("[d3d9_renderer_2d]初期化しました。");
	return true;
}

bool d3d9_renderer_2d::check_caps()
{
	bool r = true;

	// 頂点シェーダー2.0を要求
	if(draw_caps().vertex_shader_ver() < D3DVS_VERSION(2,0))
	{
		logger::fatalln("[d3d9_renderer_2d]必要な頂点シェーダーバージョン(2.0)を満たしていません。：" + to_str(D3DSHADER_VERSION_MAJOR(draw_caps().vertex_shader_ver())) 
																									  + "_" + to_str(D3DSHADER_VERSION_MINOR(draw_caps().vertex_shader_ver())));
		r = false;
	}

	// ピクセルシェーダー2.0を要求
	if(draw_caps().pixel_shader_ver() < D3DPS_VERSION(2,0))
	{
		logger::fatalln("[d3d9_renderer_2d]必要なピクセルシェーダーバージョン(2.0)を満たしていません。：" + to_str(D3DSHADER_VERSION_MAJOR(draw_caps().pixel_shader_ver())) 
																										  + "_" + to_str(D3DSHADER_VERSION_MINOR(draw_caps().pixel_shader_ver())));
		r = false;
	}

	// テクスチャバインド数は2以上
	if(draw_caps().max_texture_bind_num() < 2)
	{
		logger::fatalln("[d3d9_renderer_2d]必要なテクスチャバインド数(2)を満たしていません。：" + to_str(draw_caps().max_texture_bind_num()));
		r = false;
	}

	// テクスチャサイズが2048x2048以上
	if(draw_caps().max_texture_width()<2048 && draw_caps().max_texture_height()<2048)
	{
		logger::fatalln("[d3d9_renderer_2d]必要なテクスチャサイズ(2048×2048)を満たしていません。：" + to_str(draw_caps().max_texture_width()) + "x" + to_str(draw_caps().max_texture_height()));
		r = false;
	}

	// ストリーム数が4以上
	if(draw_caps().max_streams() < 4)
	{
		logger::fatalln("[d3d9_renderer_2d]必要なストリーム数(4)を満たしていません。：" + to_str(draw_caps().max_streams()));
		r = false;
	}

	// 1 DrawCallに必要な頂点数作れるか。1スプライトあたり4頂点使う
	if(draw_caps().max_vertex_index() < nMaxDrawCallPrimitiveNum_*4)
	{
		logger::fatalln("[d3d9_renderer_2d]1 DrawCallに必要な頂点数(" + to_str(nMaxDrawCallPrimitiveNum_) + "*4)を満たしていません。：" + to_str(draw_caps().max_vertex_index()));
		r = false;
	}

	// 1 DrawCallのプリミティブ数が足りているか。1スプライトあたり2プリミティブ使う
	if(draw_caps().max_primitive_count() < nMaxDrawCallPrimitiveNum_*2)
	{
		logger::fatalln("[d3d9_renderer_2d]1 DrawCallに必要なプリミティブ数(" + to_str(nMaxDrawCallPrimitiveNum_) + "*2)を満たしていません。：" + to_str(draw_caps().max_primitive_count()));
		r = false;
	}

	return r;
}

namespace{

template<typename index_type>
inline bool write_index(LPDIRECT3DINDEXBUFFER9	pIndexBuffer, uint32_t nMaxDrawCallSpriteNum)
{
	// 0:左上 1:右上 2:左下 3:右下のＺ並び。時計回りなので、0,1,2 1,3,2 が基本
	// 基本インデックス配列作成。1つの四角ポリゴンにつき、6インデックス使う

	index_type* pIndexBuf;
	HRESULT r = pIndexBuffer->Lock(0,0, reinterpret_cast<void**>(&pIndexBuf),0);
	if(!check_hresult(r, "[d3d9_renderer_2d]インデックスバッファをロックできませんでした。")) return false;

	for(index_type i=0; i<nMaxDrawCallSpriteNum; ++i)
	{
		pIndexBuf[0+i*6] = 0 + i*4;
		pIndexBuf[1+i*6] = 1 + i*4;
		pIndexBuf[2+i*6] = 2 + i*4;
		pIndexBuf[3+i*6] = 1 + i*4;
		pIndexBuf[4+i*6] = 3 + i*4;
		pIndexBuf[5+i*6] = 2 + i*4;
	}

	pIndexBuffer->Unlock();
	return true;
}

} // namespace end

bool d3d9_renderer_2d::init_vertex()
{
	// 頂点宣言
	D3DVERTEXELEMENT9 decl[] =
	{
		{0, 0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{1, 0, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		{2, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
		{3, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,	   1},
		D3DDECL_END()
	};

	HRESULT r;
	r = device()->CreateVertexDeclaration(decl, &pVertexDecl_);
	if(!check_hresult(r, "[d3d9_renderer_2d]頂点宣言が作成できませんでした。")) return false;
	

	// 頂点バッファ確保
	// 毎フレーム書き込みが発生するので、DYNAMICで確保する

	// 位置頂点 (x,y,z)×4頂点×スプライト数
	r = device()->CreateVertexBuffer(sizeof(POS_VERTEX)*4*nMaxDrawSpriteNum_, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pPosVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]Pos頂点バッファが確保できませんでした。")) return false;

	// UV頂点 (u,v)×4頂点×スプライト数
	r = device()->CreateVertexBuffer(sizeof(UV_VERTEX)*4*nMaxDrawSpriteNum_, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pUvVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]UV頂点バッファが確保できませんでした。")) return false;

	// Color ARGB(a,r,g,b)×4頂点×スプライト数
	r = device()->CreateVertexBuffer(sizeof(COLOR_VERTEX)*4*nMaxDrawSpriteNum_, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pColorVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]Color頂点バッファが確保できませんでした。")) return false;

	// Color ARGB(a,r,g,b)×4頂点×スプライト数
	r = device()->CreateVertexBuffer(sizeof(COLOR_VERTEX)*4*nMaxDrawSpriteNum_, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pColor2Vertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]UV2頂点バッファが確保できませんでした。")) return false;

	// インデックスバッファ確保
	uint32_t	nIndexNum	= nMaxDrawCallPrimitiveNum_*6; // 1つの四角ポリゴンを扱うには6つのインデックス

	// 基本は16bit
	D3DFORMAT	indexFMT	= D3DFMT_INDEX16;
	uint32_t	nIndexSize	= sizeof(uint16_t)*nIndexNum;

	// インデックス数が16bit範囲を超えていたら、32bitに切り替え
	if(nIndexNum > UINT16_MAX)
	{
		indexFMT	= D3DFMT_INDEX32;
		nIndexSize	= sizeof(uint32_t)*nIndexNum;
	}

	// インデックスは一度書き込んだら終わりなので、DEFAULTのみ
	r = device()->CreateIndexBuffer(nIndexSize, D3DUSAGE_WRITEONLY, indexFMT, D3DPOOL_DEFAULT, &pIndexBuffer_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]インデックスバッファが確保できませんでした。：" + to_str(indexFMT==D3DFMT_INDEX16 ? 16 : 32)))
		return false;

	// インデックスへの書き込み
	if(indexFMT == D3DFMT_INDEX16)
	{
		if(!write_index<uint16_t>(pIndexBuffer_, nMaxDrawCallPrimitiveNum_))
			return false;
	}
	else
	{
		if(!write_index<uint32_t>(pIndexBuffer_, nMaxDrawCallPrimitiveNum_))
			return false;
	}

	return true;
}

bool d3d9_renderer_2d::init_effect(const string& sPath)
{
	string sEffect =
#ifdef _DEBUG
		"d3d9_renderer_2d_d.fxc"
#else
		"d3d9_renderer_2d.fxc"
#endif
	;

	LPD3DXBUFFER buffer;
	HRESULT r = D3DXCreateEffectFromFile(device(), (sPath+sEffect).c_str(), NULL, NULL, D3DXSHADER_SKIPVALIDATION, NULL, &pEffect_, &buffer);
	if(!check_hresult(r, "[d3d9_renderer_2d]エフェクト(" + sPath + sEffect +")の作成ができませんでした。"))
	{
		if(buffer)
			logger::fatalln(reinterpret_cast<char*>(buffer->GetBufferPointer()));
		
		return false;
	}

	hTechnique_		= pEffect_->GetTechniqueByName("base");
	hMatPerPixel_	= pEffect_->GetParameterByName(NULL,"matPerPixel");
	hTex_			= pEffect_->GetParameterByName(NULL,"tex");

	// ピクセル単位で描画するための変換行列
	matPerPixel_._11 =  2.0f / static_cast<float>(nRenderWidth_);
	matPerPixel_._12 =  0;
	matPerPixel_._13 =  0;
	matPerPixel_._14 =  0;

	matPerPixel_._21 =  0;
	matPerPixel_._22 = -(2.0f / static_cast<float>(nRenderHeight_));
	matPerPixel_._23 =  0;
	matPerPixel_._24 =  0;

	matPerPixel_._31 =  0;
	matPerPixel_._32 =  0;
	matPerPixel_._33 =  1.0f / fMaxZ_; // Zは0.0～fMaxZ_の範囲
	matPerPixel_._34 =  0;

	matPerPixel_._41 = -1;
	matPerPixel_._42 =  1;
	matPerPixel_._43 =  0;
	matPerPixel_._44 =  1;

	return true;
}

bool d3d9_renderer_2d::init_render_target()
{
	LPDIRECT3DDEVICE9 pDevice = device();
	HRESULT r;

	// 一時描画用頂点作成
	// 位置頂点 (x,y,z)×4頂点
	r = pDevice->CreateVertexBuffer(sizeof(POS_VERTEX)*4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pRenderPosVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]レンダーターゲット用Pos頂点バッファが確保できませんでした。"))
		return false;

	// UV頂点 (u,v)×4頂点
	r = pDevice->CreateVertexBuffer(sizeof(UV_VERTEX)*4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pRenderUvVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]レンダーターゲット用UV頂点バッファが確保できませんでした。"))
		return false;

	// Color頂点 DWORD×4頂点。これを使ってフェードを実現するかもしれないので、DYNAMIC
	r = pDevice->CreateVertexBuffer(sizeof(COLOR_VERTEX)*4, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pRenderColorVertex_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]レンダーターゲット用Color頂点バッファが確保できませんでした。"))
		return false;

	float* pVertex;
	// 位置の書き込み
	draw::RECT rect;
	rect.fLeft	= static_cast<float>(pDriver_->wnd_width() - nRenderWidth_) / 2.0f;
	rect.fTop	= static_cast<float>(pDriver_->wnd_height() - nRenderHeight_) / 2.0f;
	rect.fRight = rect.fLeft + static_cast<float>(nRenderWidth_);
	rect.fBottom = rect.fTop + static_cast<float>(nRenderHeight_);

	r = pRenderPosVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]RenderPosVertex Lock fail.")) return false;
		write_pos(pVertex, rect.fLeft, rect.fTop, rect.fRight, rect.fBottom, 100.0f);
	pRenderPosVertex_->Unlock();

	// UV位置の書き込み
	r = pRenderUvVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]RenderUvVertex Lock fail.")) return false;
		write_uv(pVertex, 0.0f, 0.0f, 1.0f, 1.0f);
	pRenderUvVertex_->Unlock();

	// カラーの書き込み
	set_screen_color(nRenderTaregetVertexColor_);

	// 一時描画テクスチャを作成

	// 絶対必要な特別なテクスチャなのでテクスチャマネージャーの管理外
	r = pDevice->CreateTexture(nRenderWidth_, nRenderHeight_, 1, D3DUSAGE_RENDERTARGET, pDriver_->wnd_format(), D3DPOOL_DEFAULT, &pRenderTexture_, NULL);
	if(!check_hresult(r, "[d3d9_renderer_2d]レンダーターゲットテクスチャを作成できませんでした。"))
		return false;

	r = pRenderTexture_->GetSurfaceLevel(0, &pRenderTarget_);
	if(!check_hresult(r, "[d3d9_renderer_2d]レンダーターゲットテクスチャのサーフェイスが取得できませんでした。"))
		return false;

	// ピクセル単位で描画するための変換行列
	matRenderPerPixel_._11 =  2.0f / static_cast<float>(pDriver_->wnd_width());
	matRenderPerPixel_._12 =  0;
	matRenderPerPixel_._13 =  0;
	matRenderPerPixel_._14 =  0;

	matRenderPerPixel_._21 =  0;
	matRenderPerPixel_._22 = -(2.0f / static_cast<float>(pDriver_->wnd_height()));
	matRenderPerPixel_._23 =  0;
	matRenderPerPixel_._24 =  0;

	matRenderPerPixel_._31 =  0;
	matRenderPerPixel_._32 =  0;
	matRenderPerPixel_._33 =  1.0f / fMaxZ_; // Zは0.0～fMaxZ_の範囲
	matRenderPerPixel_._34 =  0;

	matRenderPerPixel_._41 = -1;
	matRenderPerPixel_._42 =  1;
	matRenderPerPixel_._43 =  0;
	matRenderPerPixel_._44 =  1;

	return true;
}

void d3d9_renderer_2d::init_render()
{
	// 頂点設定
	device()->SetVertexDeclaration(pVertexDecl_);
	device()->SetIndices(pIndexBuffer_);

	// レンダーステート設定
	render_state_alpha_test(true);

	// エフェクトステート設定
	pEffect_->SetTechnique(hTechnique_);

	// 1レンダーで呼ばれるDrawIndexedPrimitive推定数
	vecDrawCall_.reserve(64);

	// バックバッファを黒にしておく
	HRESULT r = device()->BeginScene();
	if(check_hresult(r,"[d3d9_renderer_2d]InitBeginScene fail"))
	{
		device()->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,0,0,0), 1.0f, 0);
		device()->EndScene();
	}

	// バックバッファを取得
	device()->GetRenderTarget(0, &pBackBuf_);
}

} // namespace draw end
} // namespace mana end
