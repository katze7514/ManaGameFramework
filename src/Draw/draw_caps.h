#pragma once

namespace mana{
namespace draw{

class d3d9_driver;

struct video_info
{
	string		sName_;
	uint32_t	nProduct_;
	uint32_t	nVer_;
	uint32_t	nSubVer_;
	uint32_t	nBuild_;

	uint32_t	nTexRAM_; //!< テクスチャメモリ量(MB)

	video_info():nProduct_(0),nVer_(0),nSubVer_(0),nBuild_(0),nTexRAM_(0){}
};

struct render_target_info
{
	D3DFORMAT	format_;
	bool		bAlpha_;			//!< αブレンド可能
	bool		bFilter_;			//!< バイリニアフィルタなどのフィルタが可能
	bool		bVertexTexture_;	//!< 頂点テクスチャとして使える

	//! set向けオーバーライド
	bool operator<(const render_target_info& rhs)const
	{
		return format_ < rhs.format_;
	}

	render_target_info(D3DFORMAT format=D3DFMT_UNKNOWN)
		: format_(format),bAlpha_(false),bFilter_(false),bVertexTexture_(false){}
};

/*! @brief D3Dの描画情報を調査する
 *
 *  現在のディスプレイモードで使える情報を集める
 *  つまり、DisplayFormatは現在のフォーマットで固定される
 */
class d3d9_draw_caps
{
public:
	//! @defgroup draw_caps_type draw_capsで使う型定義
	//! @{

	//! フォーマット情報タプル
	typedef tuple<D3DDEVTYPE, D3DFORMAT> typeformat_tuple;
	//! デバイスタイプ情報タプル
	typedef tuple<D3DDEVTYPE, D3DFORMAT, BOOL> devicetype_tuple;
	//! マルチサンプル情報タプル
	typedef tuple<D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD> multisample_tuple;

	//! @}

	d3d9_draw_caps();

	//! @brief driver描画情報を収集する
	/*! @param[in] adapter checkに使うD3D描画アダプタ
	 *                     driverが初期化済みであること */
	bool check_driver(d3d9_driver& adapter);

	//! @brief device描画情報を収集する
	/*! @param[in] adapter checkに使うD3D描画アダプタ
	 *                     deviceが初期化済みであること */
	bool check_device(d3d9_driver& adapter);

	const video_info&				videoinfo()const{ return videoInfo_; }
	const D3DDISPLAYMODE&			curdisp()const{ return curDisp_; }
	const vector<D3DDISPLAYMODE>&	dipsmode()const{ return vecDispMode_; }

	bool	is_backbuf_format(D3DDEVTYPE type, D3DFORMAT format, BOOL bWin);
	bool	is_disp_format(D3DFORMAT format);

	bool	is_depth_format(D3DDEVTYPE type, D3DFORMAT format);
	bool	is_textrue_format(D3DDEVTYPE type, D3DFORMAT format);

	// 使用できるレベルが返って来る。使用できない時は、uninitialized
	optional<DWORD>	is_multisample(D3DDEVTYPE type, D3DFORMAT format, BOOL bWin, D3DMULTISAMPLE_TYPE multi);

	//! @brief レンダーターゲット情報を検索
	//! @return 存在しなかったら、uninitializedになる
	optional<render_target_info>	find_render_target(D3DFORMAT format);


	//! @defgroup draw_caps_devcap 描画能力検索
	//! @{
	const D3DCAPS9&	device_caps()const{ return devCap_; }

	uint32_t		vertex_shader_ver()const{ return devCap_.VertexShaderVersion; }
	uint32_t		pixel_shader_ver()const{ return devCap_.PixelShaderVersion; }

	uint32_t		max_streams()const{ return devCap_.MaxStreams; }

	uint32_t		max_texture_bind_num()const{ return devCap_.MaxSimultaneousTextures; }
	uint32_t		max_texture_render_num()const{ return devCap_.NumSimultaneousRTs; }

	//! テクスチャサイズ正方形制限か
	bool			is_limit_texsize_square()const{ return (devCap_.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)>0; }
	//! テクスチャサイズ2乗制限があるか
	bool			is_limit_texsize_pow2()const{ return (devCap_.TextureCaps & D3DPTEXTURECAPS_POW2)>0; }
	//! テクスチャサイズ条件付き2乗制限かどうか
	bool			is_nonlimit_texsize_pow2_cond()const{ return (devCap_.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)>0; }
	//! テクスチャサイズ制限がないかどうか
	bool			is_nonlimit_texsize()const{ return !is_limit_texsize_square() && !is_limit_texsize_pow2() && !is_nonlimit_texsize_pow2_cond(); }

	uint32_t		max_texture_width()const{ return devCap_.MaxTextureWidth; }
	uint32_t		max_texture_height()const{ return devCap_.MaxTextureHeight; }

	uint32_t		max_primitive_count()const{ return devCap_.MaxPrimitiveCount; }
	uint32_t		max_vertex_index()const{ return devCap_.MaxVertexIndex; }

	//! 異方性フィルタリングサポート
	bool			is_anisotropy_filter()const{ return (devCap_.RasterCaps & D3DPRASTERCAPS_ANISOTROPY)>0;}
	//! @}

private:
	//! ビデオカード情報を取得
	bool check_video_info(d3d9_driver& adapter);

	//! 現在のディスプレイ情報を取得
	bool check_cur_disp(d3d9_driver& adapter);

	//! バックバッファーとして使えるフォーマットを取得
	bool check_device_type(d3d9_driver& adapter);
	bool check_device_type_detail(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT format, BOOL bWindow);

	//! 使用可能なディスプレイモードを取得
	void check_disp_mode(d3d9_driver& adapter);
	void check_disp_mode_detail(D3DFORMAT format, d3d9_driver& adapter);

	//! 使用可能な各種フォーマットを取得
	void check_adapter_format(d3d9_driver& adapter);
	void check_adapter_format_depth(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat);
	void check_adapter_format_texture(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat);
	void check_adapter_format_render(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat);

	//! 使用可能なマルチサンプル情報
	void check_multisample(d3d9_driver& adapter);
	void check_multisample_detail(d3d9_driver& adapter, D3DDEVTYPE type, D3DFORMAT checkformat, BOOL bWin);

private:
	//! ビデオカード情報
	video_info		videoInfo_;

	//! 現在の画面情報
	D3DDISPLAYMODE	curDisp_;

	//! 使用可能なバックバッファフォーマット
	/*! 0:HAL、1:REF */
	vector<devicetype_tuple> vecBackbufFormat_;

	//! 使用可能なディスプレイモード
	vector<D3DDISPLAYMODE>	vecDispMode_;

	//! @brief 使用可能なディスプレイフォーマット
	/*! 使用可能なディスプレイモードからフォーマットだけを集めたもの */
	flat_set<D3DFORMAT>		setDispFormat_;

	//! 使用可能な深度ステンシルフォーマット
	vector<typeformat_tuple>	vecDepthStencilFromat_;

	//! 使用可能なテクスチャフォーマット
	vector<typeformat_tuple>	vecTextureFormat_;

	//! 使用可能なレンダーターゲット情報
	flat_set<render_target_info> setRenderTargetInfo_;

	//! 使用可能なマルチサンプル情報
	vector<multisample_tuple>	vecMultiSampleInfo_;

	//! デバイス能力
	D3DCAPS9	devCap_;
};

} // namespace draw end
} // namespace mana end
