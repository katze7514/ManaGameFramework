/*
 * d39d_2d_renderクラスのレンダリングに関わるメソッド実装
 */
#include "../mana_common.h"

#include "../Timer/elapsed_timer.h"

#include "renderer_2d_util.h"
#include "d3d9_renderer_2d.h"

namespace mana{
namespace draw{

///////////////////////////////////////////
// 一時描画テクスチャ系
///////////////////////////////////////////
void d3d9_renderer_2d::set_screen_color(D3DCOLOR color)
{
	if(nRenderTaregetVertexColor_!=color)
	{
		nRenderTaregetVertexColor_  = color;

		DWORD* pColor;
		HRESULT r = pRenderColorVertex_->Lock(0,0,reinterpret_cast<void**>(&pColor),D3DLOCK_DISCARD);
		if(!check_hresult(r,"[d3d9_2d_render]RenderColorVertex Lock fail.")) return;
			write_color(pColor, nRenderTaregetVertexColor_);
		pRenderColorVertex_->Unlock();
	}
}

///////////////////////////////////////////
// レンダーステート系
///////////////////////////////////////////
void d3d9_renderer_2d::render_state_alpha_test(bool bEnable)
{
	if(bEnable)
	{
		device()->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		device()->SetRenderState(D3DRS_ALPHAREF,		10);
		device()->SetRenderState(D3DRS_ALPHAFUNC,		D3DCMP_GREATEREQUAL);
	}
	else
	{
		device()->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	}
}

void d3d9_renderer_2d::render_state_wire_frame(bool bEnable)
{
	if(bEnable)
		device()->SetRenderState(D3DRS_FILLMODE,	D3DFILL_WIREFRAME);
	else
		device()->SetRenderState(D3DRS_FILLMODE,	D3DFILL_SOLID);
}

///////////////////////////////////////////
// レンダー系
///////////////////////////////////////////
//timer::elapsed_timer timer;

bool d3d9_renderer_2d::begin_scene()
{
	if(!pDriver_) return false;
	return driver()->begin_scene();
}

bool d3d9_renderer_2d::end_scene()
{
	if(!pDriver_) return false;
	return driver()->end_scene();
}

bool d3d9_renderer_2d::render()
{
	if(!pDriver_ || !device()) return false;

	++nRenderCounter_;

	//timer.start();

	HRESULT r;

	// コマンドソート
	renderQueue_.sort_cmd();

	// レンダーテクスチャ設定
	device()->SetRenderTarget(0, pRenderTarget_);

	UINT numPass;
	r = pEffect_->Begin(&numPass, D3DXFX_DONOTSAVESTATE|D3DXFX_DONOTSAVESHADERSTATE);
	if(check_hresult(r))
	{
		if(render_ready())
		{// レンダー準備に失敗したら最後に成功したフレームの絵を出し続ける
			//render_state_wire_frame(true);

			r = device()->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, nRenderTaregetColor_, 1.0f, 0);
			render_inner();
		}

		// バックバッファを戻す
		device()->SetRenderTarget(0, pBackBuf_);

		//render_state_wire_frame(false);

		device()->Clear(0, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_ARGB(255,0,0,0), 1.0f, 0);
			
		// 一時テクスチャをバックバッファに描画
		pEffect_->SetTexture(hTex_,  pRenderTexture_);
		device()->SetStreamSource(0, pRenderPosVertex_,		0, sizeof(POS_VERTEX));
		device()->SetStreamSource(1, pRenderUvVertex_,		0, sizeof(UV_VERTEX));
		device()->SetStreamSource(2, pRenderColorVertex_,	0, sizeof(COLOR_VERTEX));
		device()->SetStreamSource(3, NULL, 0, 0);
			
		// エフェクトステート設定
		pEffect_->SetMatrix(hMatPerPixel_, &matRenderPerPixel_);

		r = pEffect_->BeginPass(MODE_TEX_COLOR1);
		if(check_hresult(r))
		{
			device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
			pEffect_->EndPass();
		}

		pEffect_->End();
	}

	//timer.end();
	//logger::traceln("EndScene " + to_str_s(nFrameCounter_) + to_str(timer.elasped_nano()));

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	if(tex_manager().texture_use_vram() > nMaxTextureVram_)
		nMaxTextureVram_ = tex_manager().texture_use_vram();
#endif

	//timer.end();
	//logger::traceln("Presnt " + to_str_s(nFrameCounter_) + to_str(timer.elasped_nano()));

	// スクリーンショットフラグが立ってたら、描画の最後に出力する
	if(bScreenShot_)
	{
		D3DXSaveSurfaceToFile((sScreenShotHeader_ + "_" + to_str(nRenderCounter_) + ".bmp").c_str(), D3DXIFF_BMP, pBackBuf_, NULL, NULL);
		bScreenShot_ = false;
	}

	renderQueue_.clear_cmd();

	return true;
}


///////////////////////////////////////////
// レンダー実体系
///////////////////////////////////////////
bool d3d9_renderer_2d::render_ready()
{
	// 描画コマンドバッファから頂点に情報を書き込みつつ
	// 描画プリミティブ数などを保存する
	if(renderQueue_.is_cmd_empty()) return false; // 描画コマンドなしなので終了

	// DrawCallパラメタをクリア
	vecDrawCall_.clear();

	// 頂点バッファへデータを書き込む
	BOOST_SCOPE_EXIT( this_ )
	{// UnLockを仕込む
		this_->pPosVertex_->Unlock();
		this_->pUvVertex_->Unlock();
		this_->pColorVertex_->Unlock();
		this_->pColor2Vertex_->Unlock();
	}
	BOOST_SCOPE_EXIT_END

	float* pPos;
	float* pUV;
	float* pColor;
	float* pColor2;
	HRESULT r = pPosVertex_->Lock(0,0,reinterpret_cast<void**>(&pPos),D3DLOCK_DISCARD);
	if(!check_hresult(r,"[d3d9_renderer_2d]位置頂点バッファがロックできませんでした。")) return false;

	r = pUvVertex_->Lock(0,0,reinterpret_cast<void**>(&pUV),D3DLOCK_DISCARD);
	if(!check_hresult(r,"[d3d9_renderer_2d]UV頂点バッファがロックできませんでした。")) return false;

	r = pColorVertex_->Lock(0,0,reinterpret_cast<void**>(&pColor),D3DLOCK_DISCARD);
	if(!check_hresult(r,"[d3d9_renderer_2d]カラー頂点バッファがロックできませんでした。")) return false;

	r = pColor2Vertex_->Lock(0,0,reinterpret_cast<void**>(&pColor2),D3DLOCK_DISCARD);
	if(!check_hresult(r,"[d3d9_renderer_2d]カラー2頂点バッファがロックできませんでした。")) return false;

	// スプライトコマンドバッファに合わせて書き込み
	uint32_t nStartVertex = 0;

	draw_call_param	drawParam;
	drawParam.nRendetTarget_ = cmd::BACK_BUFFER_ID;
	drawParam.nTexture_		 = 0;
	drawParam.eMode_		 = MODE_NONE;
	drawParam.nStartVertex_  = nStartVertex;
	drawParam.nVertexNum_	 = 0;

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	uint32_t nDrawSpriteCounter=0; // 実際に使われたスプライト数カウンタ
#endif

	renderer_sprite_queue::cmd_iterator it = renderQueue_.iterator();
	while(!it.is_end())
	{
		render_target_cmd& rtCmd = it.next();

		// レンダーターゲット変更！
		if(drawParam.nRendetTarget_ != rtCmd.nRenderTargetID_)
		{
			if(drawParam.nVertexNum_>0)
			{// 前のレンダーターゲットコマンドがあるなら入れておく
				nStartVertex += drawParam.nVertexNum_;
				vecDrawCall_.emplace_back(drawParam);
			}
			// drawParam初期化
			drawParam.nRendetTarget_ = rtCmd.nRenderTargetID_;
			drawParam.nTexture_		 = 0;
			drawParam.eMode_		 = MODE_NONE;
			drawParam.nStartVertex_  = nStartVertex;
			drawParam.nVertexNum_	 = 0;
		}

		// 不透明
		for(const sprite_param& cmd : rtCmd.vecOpaqueCmd_)
		{
			if(!sprite_param_to_draw_call_cmd(cmd, drawParam, nStartVertex, pPos, pUV, pColor, pColor2))
				goto END;

		#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
			++nDrawSpriteCounter;
		#endif
		}

		// 半透明
		for(const sprite_param& cmd : rtCmd.vecTransparentCmd_)
		{
			if(!sprite_param_to_draw_call_cmd(cmd, drawParam, nStartVertex, pPos, pUV, pColor, pColor2))
				goto END;

		#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
			++nDrawSpriteCounter;
		#endif
		}
	}

END:
	if(drawParam.nVertexNum_>0)
		vecDrawCall_.emplace_back(drawParam);

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	// 1レンダーで使うスプライト最大数
	if(nDrawSpriteCounter>nMaxDrawSpriteCounter_)
		nMaxDrawSpriteCounter_=nDrawSpriteCounter;
#endif

	return true;
}

void d3d9_renderer_2d::render_inner()
{
#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	uint32_t nDrawCallCounter=0;
#endif

	device()->SetStreamSource(0, pPosVertex_,	 0, sizeof(POS_VERTEX));
	device()->SetStreamSource(1, pUvVertex_,	 0, sizeof(UV_VERTEX));
	device()->SetStreamSource(2, pColorVertex_,	 0, sizeof(COLOR_VERTEX));
	device()->SetStreamSource(3, pColor2Vertex_, 0, sizeof(COLOR_VERTEX));

	// エフェクトステート設定
	pEffect_->SetMatrix(hMatPerPixel_, &matPerPixel_);

	HRESULT  r;
	LPDIRECT3DSURFACE9	pCurRenderTarget	= nullptr;
	uint32_t			nCurRederTargetID	= cmd::BACK_BUFFER_ID;
	uint32_t			nCurTextureID		= 0;
	draw_mode			eCurMode			= MODE_NONE;
	bool				bBeginPass			= false;

	// DrawCallパラメータ配列に応じて描画する
	for(const draw_call_param& param : vecDrawCall_)
	{
		// レンダーターゲット変更
		if(param.nRendetTarget_!=nCurRederTargetID)
		{
			safe_release(pCurRenderTarget);

			if(param.nRendetTarget_!=cmd::BACK_BUFFER_ID)
			{// バックバッファ以外はテクスチャマネージャーが持っているはず
				LPDIRECT3DTEXTURE9 pRTTex = texManager_.texture(param.nRendetTarget_);
				if(!pRTTex) continue;

				r = pRTTex->GetSurfaceLevel(0, &pCurRenderTarget);
				if(!check_hresult(r)) continue;
				device()->SetRenderTarget(0, pCurRenderTarget);
			}
			else
			{// バックバッファに戻す
				device()->SetRenderTarget(0, pRenderTarget_);
			}

			nCurRederTargetID = param.nRendetTarget_;
		}

		// テクスチャ変更
		if(param.nTexture_!=nCurTextureID)
		{
			LPDIRECT3DTEXTURE9 pTex = texManager_.texture(param.nTexture_);
			if(pTex)
			{
				pEffect_->SetTexture(hTex_, pTex);
				if(bBeginPass)
				{
					r = pEffect_->CommitChanges();
					if(!check_hresult(r)) continue;
				}
			}
			nCurTextureID = param.nTexture_;
		}

		// 描画モード変更
		if(param.eMode_!=eCurMode)
		{
			if(bBeginPass)
			{
				pEffect_->EndPass();
				bBeginPass = false;
			}

			r = pEffect_->BeginPass(param.eMode_);
			if(!check_hresult(r)) continue;

			bBeginPass = true;
			eCurMode = param.eMode_;
		}

		// 描画！
		device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, param.nStartVertex_, 0, param.nVertexNum_, 0, param.nVertexNum_/2);

	#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
		++nDrawCallCounter;

		// 1回のDrawCallで使われるスプライト最大数
		if(param.nVertexNum_/4 > nMaxDrawCallSpriteCounter_)
			nMaxDrawCallSpriteCounter_ = param.nVertexNum_/4;
	#endif
	}

	if(bBeginPass)
	{
		pEffect_->EndPass();
		bBeginPass = false;
	}
	safe_release(pCurRenderTarget);

#ifdef MANA_D3D9RENDERER2D_RENDER_COUNT
	// 1レンダーで呼ばれるDrawCall最大数
	if(nDrawCallCounter > nMaxDrawCallCounter_)
		nMaxDrawCallCounter_ = nDrawCallCounter;
#endif
}

bool d3d9_renderer_2d::sprite_param_to_draw_call_cmd(const sprite_param& cmd, draw_call_param& drawParam, uint32_t& nStartVertex, float* pPos, float* pUV, float* pColor, float* pColor2)
{
	if((nStartVertex+drawParam.nVertexNum_)>nMaxDrawSpriteNum_*4)
	{// 最大頂点数を超えたら終了
		logger::warnln("[d3d9_renderer_2d]頂点バッファが満杯です。");
		return false;
	}

	// テクスチャと描画モードの変更検出
	if(drawParam.nTexture_!=cmd.nTextureID_ || drawParam.eMode_!=cmd.mode_)
	{
		if(drawParam.nVertexNum_>0)
		{
			nStartVertex += drawParam.nVertexNum_;
			vecDrawCall_.emplace_back(drawParam);
		}

		drawParam.nTexture_		= cmd.nTextureID_;
		drawParam.eMode_		= cmd.mode_;
		drawParam.nStartVertex_ = nStartVertex;
		drawParam.nVertexNum_	= 0;
	}

	// 頂点バッファ書き込み
	uint32_t v_pos = drawParam.nStartVertex_+drawParam.nVertexNum_;

	uint32_t size = sizeof(POS_VERTEX)*(4*nMaxDrawSpriteNum_ - (v_pos));
	memcpy_s(&pPos[3*v_pos], size, cmd.pos_, sizeof(POS_VERTEX)*4);
	
	// UVバッファ書き込み
	// UV変換
	auto& info = texManager_.texture_size(cmd.nTextureID_);
	UV_VERTEX uv[4];

	for(uint32_t i=0; i<4; ++i)
	{
		uv[i].fU = cmd.uv_[i].fU / static_cast<float>(info.get<0>());
		uv[i].fV = cmd.uv_[i].fV / static_cast<float>(info.get<1>());
	}

	size = sizeof(UV_VERTEX)*(4*nMaxDrawSpriteNum_ - v_pos);
	memcpy_s(&pUV[2*v_pos], size, uv, sizeof(UV_VERTEX)*4);

	// COLORバッファ書き込み
	size = sizeof(COLOR_VERTEX)*(4*nMaxDrawSpriteNum_ - v_pos);
	memcpy_s(&pColor[v_pos],  size, cmd.color_[0], sizeof(COLOR_VERTEX)*4);
	memcpy_s(&pColor2[v_pos], size, cmd.color_[1], sizeof(COLOR_VERTEX)*4);

	// VertexNum増加
	drawParam.nVertexNum_ += 4;

	return true;
}


///////////////////////////////////////////
// レンダー実験系
///////////////////////////////////////////
#ifdef MANA_DEBUG

//////////////////////////////
// ポリゴンだけ
//////////////////////////////
void d3d9_renderer_2d::vertex_ready()
{
	float* pVertex;
	HRESULT r = pPosVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]PosVertex Lock fail")) return;

	pVertex = write_pos_vertex(pVertex,   0.0f,   0.0f,  50.0f,  50.0f, 1.0f);
	pVertex = write_pos_vertex(pVertex, 100.0f, 100.0f, 200.0f, 200.0f, 1.0f);
	pVertex = write_pos_vertex(pVertex, 500.0f, 300.0f, 510.0f, 310.0f, 1.0f);

	pPosVertex_->Unlock();

	DWORD* pColor;
	r = pColorVertex_->Lock(0,0,reinterpret_cast<void**>(&pColor),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]PosVertex Lock fail")) return;

	pColor = write_color_vertex(pColor, D3DCOLOR_ARGB(255,255,0,0));
	pColor = write_color_vertex(pColor, D3DCOLOR_ARGB(255,255,255,0));
	pColor = write_color_vertex(pColor, D3DCOLOR_ARGB(255,255,0,255));

	pColorVertex_->Unlock();
}

void d3d9_renderer_2d::vertex_draw()
{	
	device()->SetStreamSource(0, pPosVertex_, 0, sizeof(POS_VERTEX));
	//device()->SetStreamSource(1, pUv2Vertex_, 0, sizeof(UV_VERTEX));
	device()->SetStreamSource(2, pColorVertex_, 0, sizeof(COLOR_VERTEX));
	device()->SetStreamSource(3, NULL, 0, 0);

	HRESULT r = pEffect_->BeginPass(MODE_PLY_COLOR);
	if(check_hresult(r))
	{
		device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 12, 0, 6);
		pEffect_->EndPass();
	}
}

//////////////////////////////
// テクスチャ付き
//////////////////////////////
void d3d9_renderer_2d::tex_ready()
{
	float* pVertex;
	HRESULT r;

	r = pPosVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]PosVertex Lock fail")) return;		
		pVertex = write_pos_vertex(pVertex, 10.0f, 10.0f, 138.0f, 138.0f, 1.0f);
		pVertex = write_pos_vertex(pVertex, 400.0f, 200.0f, 600.0f, 400.0f, 1.0f);
	pPosVertex_->Unlock();

	r = pUvVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]UvVertex Lock fail")) return;
		pVertex = write_uv_vertex(pVertex, 0.0f, 0.0f, 1.0f, 1.0f);
		pVertex = write_uv_vertex(pVertex, 0.0f, 0.0f, 1.0f, 1.0f);
	pUvVertex_->Unlock();
}

void d3d9_renderer_2d::tex_draw()
{
	device()->SetStreamSource(0, pPosVertex_, 0, sizeof(POS_VERTEX));
	device()->SetStreamSource(1, pUvVertex_,  0, sizeof(UV_VERTEX));
	device()->SetStreamSource(2, NULL, 0, 0);
	device()->SetStreamSource(3, NULL, 0, 0);

	LPDIRECT3DTEXTURE9 pTexture = texManager_.texture("test");
	if(pTexture)
	{
		pEffect_->SetTexture(hTex_, pTexture);

		HRESULT r = pEffect_->BeginPass(MODE_TEX_BLEND);
		if(check_hresult(r,"[d3d9_renderer_2d]BeginPass Blend fail"))
		{
			device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 4);
			pEffect_->EndPass();
		}
	}
}

void d3d9_renderer_2d::tex_color_ready()
{
	tex_ready();

	DWORD* pVertex;
	HRESULT r;

	r = pColorVertex_->Lock(0,0,reinterpret_cast<void**>(&pVertex),0);
	if(!check_hresult(r,"[d3d9_renderer_2d]PosVertex Lock fail")) return;		
		pVertex = write_color_vertex(pVertex, D3DCOLOR_ARGB(255,200,200,200));
		pVertex = write_color_vertex(pVertex, D3DCOLOR_ARGB(255,255,0,0), D3DCOLOR_ARGB(255,255,0,0), D3DCOLOR_ARGB(255,255,255,255), D3DCOLOR_ARGB(255,255,255,255));
	pColorVertex_->Unlock();
}

void d3d9_renderer_2d::tex_color_draw()
{
	LPDIRECT3DTEXTURE9 pTexture = texManager_.texture("test");
	if(pTexture)
	{
		pEffect_->SetTexture(hTex_, pTexture);
		device()->SetStreamSource(0, pPosVertex_,  0, sizeof(POS_VERTEX));
		device()->SetStreamSource(1, pUvVertex_,   0, sizeof(UV_VERTEX));
		device()->SetStreamSource(2, pColorVertex_,0, sizeof(COLOR_VERTEX));
		device()->SetStreamSource(3, NULL, 0, 0);

		HRESULT r = pEffect_->BeginPass(MODE_TEX_COLOR_MUL);
		if(check_hresult(r,"[d3d9_renderer_2d]BeginPass Alpha Blend fail"))
		{
			device()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 4);
			pEffect_->EndPass();
		}
	}
}

#endif // MANA_DEBUG

} // namespace draw end
} // namespace mana end
