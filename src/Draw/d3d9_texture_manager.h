#pragma once

#include "../Utility/id_manger.h"

namespace mana{
namespace draw{

class d3d9_driver;

//! テクスチャーマネージャーで管理されるテクスチャ情報
struct tex_info
{
	typedef function<void(const tex_info&)> release_hadler;

	LPDIRECT3DTEXTURE9		pTexture_;		//!< テクスチャー実体

	D3DXIMAGE_INFO			imageInfo_;		//!< 元になったイメージの情報
	uint32_t				nTextureWidth_,nTextureHeight_; // 実際に確保されたテクスチャサイズ
	uint32_t				nTextureSize_;	//!< テクスチャー容量（byte）
	bool					bRenderTarget_;	//!< レンダーターゲットかどうか

	string					sFilePath_;		//!< テクスチャー元ファイルのパス

	string					 sGroup_;		//!< グループID
	list<uint32_t>::iterator it_;			//!< LRU管理のためのリスト内の自位置


	release_hadler			release_;	//!< VRAM調整時のリリースやデバイスロストの際に呼ばれるハンドラ。
										//!< レンダーターゲットの際に使う。呼び出しは描画スレッドからなので注意

	tex_info():pTexture_(nullptr),nTextureWidth_(1),nTextureHeight_(1),nTextureSize_(0),bRenderTarget_(false){}
	~tex_info(){ safe_release(pTexture_); nTextureSize_=0; }
};

/*! @brief テクスチャー管理クラス
 *
 *  テクスチャーのロード・生成・デバイスロスト対応などを行う
 *  使用するVRAMに制限を掛けて、いらないデータを自動で解放することも可能
 *
 *　テクスチャ情報を介してデータのやりとりをする
 *
 *　数値テクスチャIDは、add_texture_infoした後から使用可能
 *  1から始まる。0は無効値を表す
 *
 *　それ以外のメソッドは、明示的に調整を行いたい時に使う
 */
class d3d9_texture_manager
{
public:
	typedef shared_ptr<d3d9_driver>				d3d9_driver_sptr;
	typedef unordered_map<uint32_t, tex_info>	tex_hash;
	typedef utility::id_manager<string>			tex_id_manager;

public:
	d3d9_texture_manager();
	~d3d9_texture_manager(){ fin(); }

	//! @brief 初期化
	/*! @param[in] pDriver 初期化済みD3D9ドライバー
	 *  @param[in] nReserveTextureNum 使用するテクスチャ枚数の推定値
	 *  @param[in] nTextureMaxVRAM テクスチャが使用できるVRAM容量最大値 */
	void init(const d3d9_driver_sptr& pDriver, uint32_t nReserveTextureNum=256, uint32_t nTextureMaxVRAM=0);

	//! 終了処理
	void fin();

	//! @brief テクスチャ情報を登録。テクスチャ実体はテクスチャを取得する時に生成される
	/*! @param[in] sID テクスチャID
	 *  @param[in] sFilePath テクスチャの元ファイルパス */
	bool add_texture_info(const string& sID, const string& sFilePath, const string& sGroup="");

	//! @brief レンダーテクスチャ情報を登録
	bool add_texture_info(const string& sID, uint32_t nWidth, uint32_t nHeight, D3DFORMAT format, const string& sGroup="", const tex_info::release_hadler& handler=[](const tex_info&){});

	//! テクスチャ情報を削除。テクスチャ実体などが存在してた場合は解放する
	void remove_texture_info(uint32_t nID);
	void remove_texture_info(const string& sID){ auto n = texture_id(sID); if(n){ remove_texture_info(*n); }}

	void remove_texture_info_group(const string& sGroup);

	//! テクスチャ情報を取得。存在しない時は、nID_が0になっている
	const tex_info& texture_info(uint32_t nID);
	const tex_info& texture_info(const string& sID){ auto n = texture_id(sID); if(n) return texture_info(*n); else return std::move(tex_info()); }

	//! テクスチャが登録されているかどうか
	bool is_texture_info(uint32_t nID)const;
	bool is_texture_info(const string& sID){ auto n = texture_id(sID); if(n) return is_texture_info(*n); else return false; }

	//! テクスチャの数値IDから文字列IDを取得
	const string&		texture_id(uint32_t nID){ return texIdMgr_.id(nID); }
	//! テクスチャの文字列IDから数値IDを取得
	optional<uint32_t>  texture_id(const string& sID){ return texIdMgr_.id(sID); }
	
	//! テクスチャーを取得する。テクスチャーが作成されていなかったら作成する
	LPDIRECT3DTEXTURE9	texture(uint32_t nID);
	LPDIRECT3DTEXTURE9	texture(const string& sID){ auto n = texture_id(sID); if(n) return texture(*n); else return nullptr; }

	//! 実際に確保されたテクスチャサイズ(幅・高さ)を取得。テクスチャが存在していなかったら幅・高さが0で返ってくる
	const tuple<uint32_t,uint32_t> texture_size(uint32_t nID);
	const tuple<uint32_t,uint32_t> texture_size(const string& sID){ auto n = texture_id(sID); if(n) return texture_size(*n); else return std::move(make_tuple<uint32_t,uint32_t>(0,0)); }

	//! テクスチャの元画像の情報を取得
	const D3DXIMAGE_INFO&	texture_image_info(uint32_t nID);
	const D3DXIMAGE_INFO&	texture_image_info(const string& sID){ auto n = texture_id(sID); if(n) return texture_image_info(*n); else return std::move(D3DXIMAGE_INFO()); }

	//! @brief テクスチャ情報ファイルを読み込んで、テクスチャ情報を登録する
	/*! @param[in] sFilePath テクスチャ情報ファイルへのパス。もしくはテクスチャ情報文字列 
	 *  @param[in] bFile sFilePathの種別を表す。trueならファイルへのパス、falseなら文字列 */
	bool	 load_texture_info_file(const string& sFilePath, bool bFile=true);

	//! デバイスロスト対応
	bool	 device_lost();

	//! テクスチャのVRAM容量最大値を設定
	void	 set_texture_max_vram(uint32_t nTextureMaxVRAM){ nTextureMaxVRAM_=nTextureMaxVRAM; }

	//! 使用してるテクスチャー総容量（目安）を取得
	uint32_t texture_use_vram()const{ return nTextureVRAM_; }


	///// 以下ユーティリティーメソッド

	//! @brief テクスチャ情報を登録しつつ、即座にテクスチャを生成する
	/*! @param[in] sID 登録するテクスチャID 
	 *  @param[in] sFilePath テクスチャの元になるファイルへのパス */
	LPDIRECT3DTEXTURE9 create_texture(const string& sID, const string& sFilePath, const string& sGroup="");

	//! @brief テクスチャ情報を登録しつつ、即座にレンダーターゲットとなるテクスチャを生成する
	LPDIRECT3DTEXTURE9 create_texture_render_target(const string& sID, uint32_t nWidth, uint32_t nHeight, D3DFORMAT format, const string& sGroup="", const tex_info::release_hadler& handler=[](const tex_info&){});

	//! @brief テクスチャを解放する。テクスチャ情報は削除しない。つまり、VRAMの解放だけ行う
	/*! @param[in] nID 解放対象となるテクスチャID */
	void release_texture(uint32_t nID);
	void release_texture(const string& sID){ auto n = texture_id(sID); if(n) release_texture(*n); }

	void release_texture_group(const string& sGroup);

	//! 指定されたテクスチャをPNGにしてファイルに書き出す。拡張子は自動で付与されます
	bool write_texture_image(const string& sID, const string& sFilePath);

private:
	//! @brief 実際にテクスチャを生成する
	/*! @param[in] nID 対応するテクスチャID
	 *  @param[in,out] info 設定されている情報に基づきテクスチャを生成し、infoの中に設定する */
	bool create_texture_inner(uint32_t nID, tex_info& info);

	//! @defgroup texture_manager_vram テクスチャ容量の管理
	//! @{
	//! @brief テクスチャ容量の調整を行う。テクスチャが追加された時に呼ぶ
	/*! @param[in] info 追加されたばかりのテクスチャ情報 */
	void vram_manage_fetch(uint32_t nID, tex_info& info);

	//! @brief テクスチャ容量の調整を行う。テクスチャが解放された時に呼ぶ
	/*! @param[in] info 解放されようとしているのテクスチャ情報 */
	void vram_manage_release(tex_info& info);

	//! @brief テクスチャの一番古いものを解放する
	/*! @return trueなら削除できた。falseなら削除できなかった（削除するものがなかった） */
	bool vram_manage_release_old_one();
	//! @}

private:
	//! 初期化済みd3d9ドライバー
	d3d9_driver_sptr pDriver_;

	//! テクスチャー情報マップ
	tex_hash		hashTexture_;
	//! 数値IDとテクスチャID対応テーブル
	tex_id_manager	texIdMgr_;

	//! 使用しているVRAM容量byte（目安）
	uint32_t		nTextureVRAM_;
	//! TextureVRAM使用の最大値byte（目安） 0の時は制限無し
	uint32_t		nTextureMaxVRAM_;

	//! 使用中のテクスチャ管理（LRU処理）。数値IDを入れる。前にあるほど古い
	//! レンダーターゲットは対象外なので注意
	list<uint32_t>	listTexture_;

private:
	NON_COPIABLE(d3d9_texture_manager);

#ifdef MANA_DEBUG
public:
	// 再定義フラグ。再定義フラグをtrueにしておくと、add_infoなどでデータ上書きされるようになる
	// 開発中のリロードのために使う
	void redefine(bool bReDefine){ bReDefine_=bReDefine; }

private:
	bool bReDefine_;

#endif // MANA_DEBUG
};

} // namespace draw end
} // namespace mana end

/*
<texture_def>
	<texture id="" src="" />
	<texture id="" src="" />
	<texture id="" src="" />
	<texture id="" src="" />
</texture_def>
 */
