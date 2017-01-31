#include "../mana_common.h"

#include "../Resource/resource_file.h"
#include "../Resource/resource_manager.h"

#include "xtal_manager.h"
#include "xtal_code.h"

namespace mana{
namespace script{

xtal_code::xtal_code():eState_(NONE),eType_(TYPE_NONE),bReload_(false),pMgr_(nullptr)
{ 
	vecReloadID_.reserve(8); 
	bind_ = [](const xtal::CodePtr& pCode, bool b){};

	err_ = [this](const string& sMes)
	{
		logger::fatal(sMes);
		logger::fatalln("[xtal_code][xtal_error]");

		// 再試行を押すと何か修正したことにして、再ロードやコンパイルを促す
		int r = MessageBox(NULL,sMes.c_str(),"Xtalエラーを修正して下さい",MB_RETRYCANCEL|MB_ICONWARNING|MB_APPLMODAL);
		return r==IDRETRY;
	};
}

enum xtal_code::state xtal_code::state()
{
	enum state e = eState_;

	if(e==COMPILING)
	{
		auto res = pMgr_->pResource_->resource(string_fw(sFile_));
		if(res)
		{
			auto r = compile_stream((*res)->buf().get(), (*res)->filesize()); 
			switch(r)
			{
			case COMP_REREAD:
				compile_resource(true);
			break;

			case COMP_SUCCESS:
				if(bReload_) reload_notify(true);
			break;
			}
		}
	}

	return eState_;
}

xtal_code& xtal_code::add_reload_id(const string_fw& sID)
{
	// 登録済みだったら何もしない
	for(auto& id : vecReloadID_)
		if(id==sID) return *this;

	vecReloadID_.emplace_back(sID);
	return *this; 
}

xtal_code& xtal_code::remove_reload_id(const string_fw& sID)
{
	std::remove_if(vecReloadID_.begin(), vecReloadID_.end(), [&sID](const string_fw& s){ return s==sID; });
	return *this;
}

bool xtal_code::compile(bool bReread)
{
	if(state()==COMPILING)
	{
		logger::warnln("[xtal_code]コンパイル中です。");
		return false;
	}

	switch(type())
	{
	case TYPE_RESOURCE:	return compile_resource(bReread);
	case TYPE_FILEPATH:	return compile_filepath();
	case TYPE_STR:		return compile_str();

	default:	
		logger::warnln("[xtal_code]ファイルタイプが設定されてません。");
	return false;
	}
}

bool xtal_code::compile_resource(bool bReread)
{
	if(!pMgr_->pResource_)
	{
		logger::warnln("[xtal_code]リソースタイプが指定されてますが、リソーマネージャーが登録されていません。");
		return false;
	}

	string_fw	resID(sFile_);
	bool		bErr=false;

	do{
		if(bReread) pMgr_->pResource_->release_cache(resID);

		auto r = pMgr_->pResource_->request(resID);
		if(r==resource_manager::FAIL)
		{
			logger::warnln("[xtal_code]リソースリクエストに失敗しました。");
			eState_=ERR;
			return false;
		}
		else if(r==resource_manager::EXIST)
		{
			auto res = pMgr_->pResource_->resource(resID);
			if(res)
			{
				compile_result c = compile_stream((*res)->buf().get(), (*res)->filesize()); 
				switch(c)
				{
				case COMP_ERR:		return false;
				case COMP_SUCCESS:	return true;

				case COMP_REREAD:
					bErr	= true;
					bReread	= true;
				break;
				}
			}
		
			logger::warnln("[xtal_code]あるはずのリソースがありませんでした。");
			eState_=ERR;
			return false;
		}
		else
		{
			eState_ = COMPILING;
			return true;
		}
	}while(bErr);

	return false;
}

bool xtal_code::compile_filepath()
{
	bool bErr=false;

	file::file_access f;
	f.set_filepath(sFile_);

	do
	{
		if(!f.open(file::file_access::READ_ALL))
		{
			if(err_("ファイルオープンエラーです。詳細はログを見て下さい。\n" + sFile_))
			{
				bErr=true;
			}
			else
			{
				logger::warnln("[xtal_code]ファイルオープンができませんでした。");
				eState_=ERR;
				return false;
			}
		}
		
		uint32_t rsize;
		int32_t r = f.read(rsize);
		if(r==file::file_access::FAIL)
		{
			if(err_("ファイルリードエラーです。詳細はログを見て下さい。\n" + sFile_))
			{
				bErr=true;
			}
			else
			{
				logger::warnln("[xtal_code]ファイルリードができませんでした。");
				eState_=ERR;
				return false;
			}
		}

		f.close();

		auto c = compile_stream(f.buf().get(), f.filesize());
		switch(c)
		{
		case COMP_ERR:		return false;
		case COMP_SUCCESS:	return true;

		case COMP_REREAD:
			bErr	= true;
		break;
		}
	}while(bErr);

	return false;
}

bool xtal_code::compile_str()
{
	if(pCode_)
	{
		call_bind(false);
		pCode_ = xtal::null;
		xtal::full_gc();
	}

	pCode_ = xtal::compile(sFile_.c_str());
	if(!check_xtal_result())
	{
		logger::warnln("[xtal_code]コンパイルに失敗しました。");
		eState_ = ERR;
		return false;
	}

	if(bReload_) pCode_->enable_redefine();
	call_bind(true);
	pCode_->call();
	if(!check_xtal_result())
	{
		logger::warnln("[xtal_code]実行に失敗しました。");
		eState_ = ERR;
		return false;
	}

	eState_ = OK;
	return true;
}

xtal_code::compile_result xtal_code::compile_stream(BYTE* p, uint32_t size)
{
	xtal::PointerStreamPtr stream = xtal::xnew<xtal::PointerStream>(p,size);
	
	if(pCode_)
	{
		call_bind(false);
		pCode_ = xtal::null;
		xtal::full_gc();
	}

	pCode_ = xtal::compile(stream, sFile_.c_str());

	string sErr;
	if(!check_xtal_result(sErr))
	{
		bool r = err_("[xtal_code]*** コンパイルエラー ***\n"+sErr);
		eState_ = ERR;
		return r ? COMP_REREAD : COMP_ERR;
	}

	if(bReload_) pCode_->enable_redefine();
	call_bind(true);
	pCode_->call();
	if(!check_xtal_result(sErr))
	{
		call_bind(false);

		bool r = err_("[xtal_code]*** 実行に失敗しました ***\n" + sErr);
		eState_ = ERR;
		return r ? COMP_REREAD : COMP_ERR;
	}

	eState_ = OK;
	return COMP_SUCCESS;
}

bool xtal_code::reload()
{
	reload_notify(false);
	if(!compile(true)) return false;
	if(eState_==OK)	reload_notify(true);
	
	return true;
}

void xtal_code::reload_notify(bool bAfter)
{
	for(auto& id : vecReloadID_)
	{
		auto xcode = pMgr_->code(id);
		if(!xcode || xcode->state()!=OK) continue;

		auto pTargetCode = xcode->code();
		auto reload_fun = pTargetCode->member(Xid(on_reload));
		if(reload_fun) reload_fun->call(pCode_, bAfter);
	}

	bReload_ = !bAfter;
}

//////////////////////////////////

bool check_xtal_result()
{
	XTAL_CATCH_EXCEPT(e)
	{
		xtal::StringPtr s = e->to_s();
		logger::warnln("[Xtal Error]" + string(s->c_str()));
		return false;
	}

	return true;
}

bool check_xtal_result(string& sMes)
{
	XTAL_CATCH_EXCEPT(e)
	{
		xtal::StringPtr s = e->to_s();
		sMes = s->c_str();
		return false;
	}

	return true;
}

} // namespace script end
} // namespace mana end
