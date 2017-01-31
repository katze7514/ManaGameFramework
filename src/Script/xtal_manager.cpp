#include "../mana_common.h"

#include <xtal_lib/xtal_winthread.h>
#include <xtal_lib/xtal_chcode.h>
#include <xtal_lib/xtal_errormessage.h>

#include "xtal_lib.h"
#include "xtal_code.h"
#include "xtal_manager.h"

namespace mana{
namespace script{

xtal_manager::~xtal_manager()
{
	fin();
}

void xtal_manager::init(const shared_ptr<resource_manager>& pResource)
{
	logger::add_out_category("XTL");

	pResource_ = pResource;

	init_lib();

	xtal::Setting setting;
	setting.ch_code_lib		= pCodeLib_;
	setting.std_stream_lib	= pStreamLib_;
	setting.filesystem_lib	= pFileSystemLib_;
	setting.thread_lib		= pThreadLib_;

	xtal::initialize(setting);

	xtal::bind_error_message();

	init_bind();

	bInit_ = true;
}

void xtal_manager::fin()
{
	if(!bInit_) return;

	mapCode_.clear();

	fin_bind();

	xtal::uninitialize();

	safe_delete(pThreadLib_);
	safe_delete(pFileSystemLib_);
	safe_delete(pStreamLib_);
	safe_delete(pCodeLib_);

	bInit_ = false; 
}

void xtal_manager::init_lib()
{
	pCodeLib_		= new_ xtal::SJISChCodeLib();
	pStreamLib_		= new_ logger_stream_lib();
	pFileSystemLib_ = nullptr;
	pThreadLib_		= new_ xtal::WinThreadLib();
}

void xtal_manager::init_bind()
{
	xtal::lib()->def(Xid(xtal_manager), xtal::cpp_class<xtal_manager>());
	xtal::lib()->def(Xid(manager), xtal::SmartPtr<xtal_manager>(this));
}

void xtal_manager::fin_bind()
{
	xtal::cpp_class<xtal_manager>()->object_orphan();
}

void xtal_manager::exec()
{
	xtal::gc();
}

//////////////////////////////////

const shared_ptr<xtal_code>& xtal_manager::create_code(const string_fw& sID)
{
	if(sID.get().empty())
	{
		logger::warnln("[xtal_manager]IDが空です。");
		return std::move(shared_ptr<xtal_code>());
	}

	auto code = make_shared<xtal_code>();
	code->set_id(sID);
	code->pMgr_ = this;

	auto r = mapCode_.emplace(sID, code);
	if(!r.second)
	{
		logger::warnln("[xtal_manager]コード情報を追加することができませんでした。");
		return std::move(shared_ptr<xtal_code>());
	}

	return r.first->second;
}

void xtal_manager::destroy_code(const string_fw& sID)
{
	auto it = mapCode_.find(sID);
	if(it==mapCode_.end()) return;

	it->second->call_bind(false);
	mapCode_.erase(it);
}

const shared_ptr<xtal_code>& xtal_manager::code(const string_fw& sID)
{
	auto it =  mapCode_.find(sID);
	if(it!=mapCode_.end()) return it->second;
	return std::move(shared_ptr<xtal_code>());
}

} // namespace script end
} // namespace mana end


XTAL_PREBIND(mana::script::xtal_manager)
{
	Xdef_ctor0();
}

XTAL_BIND(mana::script::xtal_manager)
{
}
