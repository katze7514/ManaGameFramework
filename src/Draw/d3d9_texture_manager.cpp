#include "../mana_common.h"

#include "../Concurrent/lock_helper.h"
#include "../File/file.h"

#include "renderer_2d_util.h"

#include "d3d9_driver.h"
#include "d3d9_texture_manager.h"

namespace mana{
namespace draw{

//////////////////////////////////////////////
// 初期化と終了
//////////////////////////////////////////////
d3d9_texture_manager::d3d9_texture_manager():nTextureVRAM_(0),nTextureMaxVRAM_(0)
{
#ifdef MANA_DEBUG
	bReDefine_=false;
#endif
}

void d3d9_texture_manager::init(const d3d9_driver_sptr& pDriver, uint32_t nReserveTextureNum, uint32_t nTextureMaxVRAM)
{
	pDriver_ = pDriver;
	hashTexture_.reserve(nReserveTextureNum);
	texIdMgr_.init(nReserveTextureNum, cmd::RESERVE_TEX_ID_END, "");
	set_texture_max_vram(nTextureMaxVRAM);
}

void d3d9_texture_manager::fin()
{
	hashTexture_.clear();
	listTexture_.clear();
	nTextureVRAM_ = 0;
}

//////////////////////////////////////////////
// テクスチャ情報
//////////////////////////////////////////////
bool d3d9_texture_manager::add_texture_info(const string& sID, const string& sFilePath, const string& sGroup)
{
	if(sID.empty() || sFilePath.empty()) return false;

	// 追加が出来たら数値IDを振る
	uint32_t nID = texIdMgr_.assign_id(sID);

	tex_info info;
	info.it_		= listTexture_.end();
	info.sFilePath_ = sFilePath;
	info.sGroup_	= sGroup;

	auto r = hashTexture_.emplace(nID, info);

	if(!r.second)
	{
	#ifdef MANA_DEBUG
		if(bReDefine_)
		{// リロードフラグ立ってる時は上書き
			auto& tex = r.first->second;
			if(tex.pTexture_)
			{
				if(tex.bRenderTarget_) tex.release_(tex);

				vram_manage_release(tex);
				safe_release(tex.pTexture_);
				tex.nTextureSize_ = 0;
				tex.bRenderTarget_= false;
			}

			// 新しい情報
			tex.sFilePath_	= sFilePath;
			tex.sGroup_		= sGroup;
			return true;
		}
	#endif

		texIdMgr_.erase_id(nID); // 追加したIDを削除

		// 同一ID、同一ファイルだったら登録成功扱いにする
		if(r.first->second.sFilePath_ == sFilePath)
		{
			logger::debugln("[d3d9_texture_manager]テクスチャ情報は登録済みです：" + sID);
			return true;
		}
		else
		{
			logger::warnln("[d3d9_texture_manager]テクスチャ情報を追加できませんでした：" + sID);
			return false;
		}
	}

	return true;
}

bool d3d9_texture_manager::add_texture_info(const string& sID, uint32_t nWidth, uint32_t nHeight, D3DFORMAT format, const string& sGroup, const tex_info::release_hadler& handler)
{
	if(sID.empty()) return false;

	// 数値IDを取得
	uint32_t nID = texIdMgr_.assign_id(sID);

	tex_info info;
	info.it_				= listTexture_.end();

	info.bRenderTarget_		= true;
	info.imageInfo_.Width	= nWidth;
	info.imageInfo_.Height	= nHeight;
	info.imageInfo_.Format	= format;
	info.release_			= handler;
	info.sGroup_			= sGroup;

	auto r = hashTexture_.emplace(nID, info);
	if(!r.second)
	{
	#ifdef MANA_DEBUG
		if(bReDefine_)
		{// リロードフラグ立ってる時は上書き
			auto& tex = r.first->second;
			if(tex.pTexture_)
			{
				if(tex.bRenderTarget_) tex.release_(tex);

				vram_manage_release(tex);
				safe_release(tex.pTexture_);
				tex.nTextureSize_ = 0;
				tex.bRenderTarget_= false;
			}

			// 新しい情報
			tex.bRenderTarget_		= true;
			tex.imageInfo_.Width	= nWidth;
			tex.imageInfo_.Height	= nHeight;
			tex.imageInfo_.Format	= format;
			tex.release_			= handler;
			tex.sGroup_				= sGroup;
			return true;
		}
	#endif

		texIdMgr_.erase_id(nID);
		logger::warnln("[d3d9_texture_manager]レンダーテクスチャ情報を追加できませんでした： " + sID);
		return false;
	}

	return true;
}

void d3d9_texture_manager::remove_texture_info(uint32_t nID)
{
	tex_hash::iterator it = hashTexture_.find(nID);
	if(it!=hashTexture_.end())
	{
		vram_manage_release(it->second);

		texIdMgr_.erase_id(nID);
		hashTexture_.erase(it);
	}
}

void d3d9_texture_manager::remove_texture_info_group(const string& sGroup)
{
	if(sGroup.empty()) return;

	tex_hash::iterator it = hashTexture_.begin();
	while(it!=hashTexture_.end())
	{
		if(it->second.sGroup_==sGroup)
		{
			vram_manage_release(it->second);

			texIdMgr_.erase_id(it->first);
			it = hashTexture_.erase(it);
		}
		else
		{
			++it;
		}
	}
}

const tex_info& d3d9_texture_manager::texture_info(uint32_t nID)
{
	tex_hash::iterator it = hashTexture_.find(nID);
	if(it==hashTexture_.end()) return std::move(tex_info());
	return it->second;
}

bool d3d9_texture_manager::is_texture_info(uint32_t nID)const
{
	tex_hash::const_iterator it = hashTexture_.find(nID);
	return it!=hashTexture_.end();
}

bool d3d9_texture_manager::load_texture_info_file(const string& sFilePath, bool bFile)
{/*
  *	<texture_def>
  *		<texture !id="" src="" group="" />
  *	</texture_def>
  */

	if(bFile)
		logger::infoln("[d3d9_texture_manager]テクスチャ定義ファイルを読み込みます。: " + sFilePath);
	else
		logger::infoln("[d3d9_texture_manager]テクスチャ定義ファイルを読み込みます。");

	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
	{
		logger::warnln("[d3d9_texture_manager]テクスチャ定義ファイルが読み込めませんでした。");
		return false;
	}

	using namespace p_tree;
	ptree ttree;
	xml_parser::read_xml(ss, ttree, xml_parser::no_comments);
	
	optional<ptree&> tex_def = ttree.get_child_optional("texture_def");
	if(!tex_def)
	{
		logger::warnln("[d3d9_texture_manager]テクスチャ定義ファイルのフォーマットが間違ってます。： <textrue_def>が存在しません。");
		return false;
	}
	

	uint32_t nTexNum=1;
	for(auto& it : *tex_def)
	{
		if(it.first=="texture")
		{
			string sID = it.second.get<string>("<xmlattr>.id","");
			string sSrc = it.second.get<string>("<xmlattr>.src","");
			string sGroup = it.second.get<string>("<xmlattr>.group","");
			if(!sID.empty() && !sSrc.empty())
			{
				add_texture_info(sID, sSrc, sGroup);
			}
			else
			{
				logger::warnln("[d3d9_texture_manager]" + to_str_s(nTexNum) + "個目のタグのフォーマットが間違っています");
				return false;
			}

			++nTexNum;
		}
	}

	logger::infoln("[d3d9_texture_manager]テクスチャ定義ファイルの読み込みに成功しました。: " + sFilePath);
	return true;
}

//////////////////////////////////////////////
// デバイスロスト
//////////////////////////////////////////////
bool d3d9_texture_manager::device_lost()
{
	for(auto& it : hashTexture_)
	{
		tex_info& info = it.second;

		vram_manage_release(info);
		safe_release(info.pTexture_);
		info.nTextureSize_ = 0;

		if(info.bRenderTarget_) info.release_(info);
	}

	return true;
}

//////////////////////////////////////////////
// テクスチャーの取得
//////////////////////////////////////////////
LPDIRECT3DTEXTURE9 d3d9_texture_manager::texture(uint32_t nID)
{
	tex_hash::iterator it = hashTexture_.find(nID);
	if(it!=hashTexture_.end())
	{
		tex_info& info = it->second;

		// テクスチャが作成されてなかったら作成する
		if(!info.pTexture_)
		{
			if(!create_texture_inner(nID, info))
				return nullptr;
		}
		else
		{
			vram_manage_fetch(nID, info);
		}

		return info.pTexture_;
	}

	return nullptr;
}

const tuple<uint32_t,uint32_t> d3d9_texture_manager::texture_size(uint32_t nID)
{
	tex_hash::iterator it = hashTexture_.find(nID);
	if(it!=hashTexture_.end())
	{
		tex_info& info = it->second;

		// テクスチャが作成されてなかったら作成する
		if(!info.pTexture_)
		{
			if(!create_texture_inner(nID, info))
				return std::move(make_tuple<uint32_t,uint32_t>(1,1));
		}
		else
		{
			vram_manage_fetch(nID, info);
		}

		return std::move(make_tuple<uint32_t,uint32_t>(info.nTextureWidth_, info.nTextureHeight_));
	}

	return std::move(make_tuple<uint32_t,uint32_t>(1,1));
}

const D3DXIMAGE_INFO& d3d9_texture_manager::texture_image_info(uint32_t nID)
{
	D3DXIMAGE_INFO img;
	::ZeroMemory(&img, sizeof(img));
	img.Width  = 1;
	img.Height = 1;
	img.Format = D3DFMT_UNKNOWN;

	tex_hash::iterator it = hashTexture_.find(nID);
	if(it!=hashTexture_.end())
	{
		tex_info& info = it->second;

		// テクスチャが作成されてなかったら作成する
		if(!info.pTexture_)
		{
			if(!create_texture_inner(nID, info))
				return std::move(img);
		}
		else
		{
			vram_manage_fetch(nID, info);
		}

		return info.imageInfo_;
	}

	return std::move(img);
}

//////////////////////////////////////////////
// 明示的なテクスチャの操作
//////////////////////////////////////////////
 LPDIRECT3DTEXTURE9 d3d9_texture_manager::create_texture(const string& sID, const string& sFilePath, const string& sGroup)
{
	if(!add_texture_info(sID, sFilePath, sGroup)) return nullptr;
	return texture(sID);
}

LPDIRECT3DTEXTURE9 d3d9_texture_manager::create_texture_render_target(const string& sID, uint32_t nWidth, uint32_t nHeight, D3DFORMAT format, const string& sGroup, const tex_info::release_hadler& handler)
{
	if(!add_texture_info(sID, nWidth, nHeight, format, sGroup, handler)) return nullptr;
	return texture(sID);
}

void d3d9_texture_manager::release_texture(uint32_t nID)
{
	tex_hash::iterator it = hashTexture_.find(nID);
	if(it!=hashTexture_.end())
	{
		vram_manage_release(it->second);

		safe_release(it->second.pTexture_);
		it->second.nTextureSize_ = 0;
	}
}

void d3d9_texture_manager::release_texture_group(const string& sGroup)
{
	if(sGroup.empty()) return;

	for(auto& it : hashTexture_)
	{
		if(it.second.sGroup_==sGroup)
		{
			vram_manage_release(it.second);

			safe_release(it.second.pTexture_);
			it.second.nTextureSize_ = 0;
		}
	}
}

//////////////////////////////////////////////
// テクスチャのファイルへの書き出し
//////////////////////////////////////////////
bool d3d9_texture_manager::write_texture_image(const string& sID, const string& sFilePath)
{
	auto nID = texture_id(sID);
	
	if(!nID) return false;

	tex_hash::iterator it = hashTexture_.find(*nID);
	if(it==hashTexture_.end())
	{
		logger::warnln("[d3d9_texture_manager]テクスチャ情報が存在しません。: " + sID);
		return false;
	}

	tex_info& info = it->second;
	if(!info.pTexture_)
	{
		logger::warnln("[d3d9_texture_manager]テクスチャが存在しません。: " + sID);
		return false;
	}

	HRESULT r = D3DXSaveTextureToFile((sFilePath + ".png").c_str(), D3DXIFF_PNG, info.pTexture_, NULL);
	return check_hresult(r,"[d3d9_texture_manager]ファイルに書き出しできませんでした。: " + sID); 
}

//////////////////////////////////////////////
// テクスチャの生成
//////////////////////////////////////////////
namespace{

inline uint32_t round_2_power_size(uint32_t n)
{
	if(n==0) return 0;
	
	uint32_t power = 1;
	while(power<n) power *=2;

	return power;
}

inline uint32_t calc_texture_size(uint32_t nWidth, uint32_t nHeight, D3DFORMAT format)
{
	uint32_t pixelbyte = 4;

	switch(format)
	{
	case D3DFMT_A2R10G10B10:
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		pixelbyte = 4;
	break;

	case D3DFMT_A1R5G5B5:
	case D3DFMT_X1R5G5B5:
	case D3DFMT_R5G6B5:
		pixelbyte = 2;
	break;

	case D3DFMT_R8G8B8:
		pixelbyte = 3;
	break;

	case D3DFMT_A8:
		pixelbyte = 1;
	break;

	default:
		logger::warnln("[d3d9_texture_manager]未知のテクスチャーフォーマットが渡されました。: " + to_str(format));
	break;
	}

	return nWidth*nHeight*pixelbyte;
}

} // namespace end

bool d3d9_texture_manager::create_texture_inner(uint32_t nID, tex_info& info)
{
	if(info.pTexture_)
	{
		safe_release(info.pTexture_);
		info.nTextureSize_ = 0;
	}

	HRESULT r;

	if(info.bRenderTarget_)
	{// レンダーテクスチャ生成
		D3DXIMAGE_INFO& imageInfo = info.imageInfo_;

		while(true)
		{
			r = pDriver_->device()->CreateTexture(imageInfo.Width, imageInfo.Height, 1, D3DUSAGE_RENDERTARGET, 
														imageInfo.Format, D3DPOOL_DEFAULT, &info.pTexture_, NULL);

			// ビデオメモリが足りないなら、テクスチャを解放して、再チャレンジ
			if(!(r==D3DERR_OUTOFVIDEOMEMORY && vram_manage_release_old_one()))
			{	break;	}		
		}

		function<void(HRESULT)> err = [this, &nID](HRESULT r){ logger::warnln("[d3d9_texture_manager]レンダーターゲットテクスチャが作成できませんでした。: " + texture_id(nID) + " " + to_str(r)); };
		if(!check_hresult(r,err)) return false;
	}
	else
	{// テクスチャ生成
		// 元ファイルを読み込む
		using file::file_access;

		file_access tex_file(info.sFilePath_);
		if(!tex_file.open(file_access::READ_ALL))
		{
			logger::warnln("[d3d9_texture_manager]テクスチャファイルを開けませんでした。: " + info.sFilePath_);
			return false;
		}

		uint32_t rsize=0;
		if(tex_file.read(rsize)==file_access::FAIL)
		{
			logger::warnln("[d3d9_texture_manager]テクスチャファイルを読むことができませんでした。: " + info.sFilePath_);
			return false;
		}

		while(true)
		{
			r = D3DXCreateTextureFromFileInMemoryEx(pDriver_->device(),
															reinterpret_cast<void*>(tex_file.buf().get()), tex_file.filesize(),
															D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_UNKNOWN,
															D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_FILTER_NONE,
															0, &info.imageInfo_, NULL,
															&info.pTexture_);

			// ビデオメモリが足りないなら、テクスチャを解放して、再チャレンジ
			if(!(r==D3DERR_OUTOFVIDEOMEMORY && vram_manage_release_old_one()))
			{	break;	}		
		}

		function<void(HRESULT)> err = [this, &nID](HRESULT r){ logger::warnln("[d3d9_texture_manager]テクスチャを作成することができませんでした。: " + texture_id(nID) + " " + to_str(r)); };
		if(!check_hresult(r,err)) return false;

		// テクスチャサイズは2の累乗に丸められるのでIMAGE_INFOも丸めておく
		info.nTextureWidth_  = round_2_power_size(info.imageInfo_.Width);
		info.nTextureHeight_ = round_2_power_size(info.imageInfo_.Height);

		// ファイルフォーマットがDXTだったらファイルサイズをそのままテクスチャ容量とする
		switch(info.imageInfo_.Format)
		{
		case D3DFMT_DXT1:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
			info.nTextureSize_ = tex_file.filesize();
		break;

		default: break;
		}
	}

	// テクスチャ容量（推定）を計算
	if(info.nTextureSize_==0)
	{
		info.nTextureSize_ = calc_texture_size(info.nTextureWidth_, info.nTextureHeight_, info.imageInfo_.Format);
	}

	vram_manage_fetch(nID, info);

	return true;
}

//////////////////////////////////////////////
// テクスチャ容量の調整
//////////////////////////////////////////////
void d3d9_texture_manager::vram_manage_fetch(uint32_t nID, tex_info& info)
{
	if(info.it_!=listTexture_.end())
	{// すでに使用中だったら、位置変更のために一端リストから削除
		listTexture_.erase(info.it_);
		info.it_ = listTexture_.end();
	}
	else
	{// info.it_==listTexture_.end() だったら新規追加
	 // 使用VRAMをチェックする
		nTextureVRAM_ += info.nTextureSize_;

		// 制限量を超えたら、制限内に収まるまでVRAMを解放する
		if(nTextureMaxVRAM_>0 && nTextureVRAM_>nTextureMaxVRAM_)
		{
			// 前にあるほど古いので前から消して行く
			list<uint32_t>::iterator it = listTexture_.begin();
			while(nTextureVRAM_>nTextureMaxVRAM_ 
			   && it!=listTexture_.end())
			{
				tex_hash::iterator info_it = hashTexture_.find(*it);
				if(info_it!=hashTexture_.end())
				{
					tex_info& info_release = info_it->second;

					vram_manage_release(info_release);

					safe_release(info_release.pTexture_);
					info_release.nTextureSize_ = 0;

					// レンダーターゲットだったら削除されたことを通知
					if(info_release.bRenderTarget_) info_release.release_(info_release);
				}
				it = listTexture_.erase(it);
			}
		}
	}

	// 後ろに追加
	listTexture_.emplace_back(nID);
	info.it_=listTexture_.end();
	--info.it_; // endの一つ前が自分の位置
}

void d3d9_texture_manager::vram_manage_release(tex_info& info)
{
	if(info.it_!=listTexture_.end())
	{
		listTexture_.erase(info.it_);
		info.it_ = listTexture_.end();
	}

	nTextureVRAM_ -= info.nTextureSize_;
}

bool d3d9_texture_manager::vram_manage_release_old_one()
{
	// 先頭が一番古い
	list<uint32_t>::iterator it = listTexture_.begin();
	tex_hash::iterator info_it = hashTexture_.find(*it);
	if(info_it!=hashTexture_.end())
	{
		tex_info& info_release = info_it->second;

		vram_manage_release(info_release);

		safe_release(info_release.pTexture_);
		info_release.nTextureSize_ = 0;

		// レンダーターゲットだったら削除されたことを通知
		if(info_release.bRenderTarget_) info_release.release_(info_release);

		listTexture_.erase(it);
		return true;
	}

	return false;
}

} // namespace draw end
} // namespace mana end
