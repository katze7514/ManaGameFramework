#include "../mana_common.h"

#include "../App/system_caps.h"
#include "../Memory/util_memory.h"
#include "../Concurrent/worker.h"
#include "../File/file.h"

#include "resource_file.h"
#include "resource_manager.h"

namespace mana{

resource_manager::resource_manager()
{
	mapID_.reserve(128);
	cacheResouce_.reserve(128);
}

void resource_manager::init(uint32_t nThreadNum)
{
	// 最低でも1スレッドは確保する
	if(nThreadNum==0) nThreadNum=1;

	pWorker_ = make_shared<concurrent::worker>(128);

	memory::scoped_alloc alloc(sizeof(uint32_t)*nThreadNum);
	uint32_t* anAffine = reinterpret_cast<uint32_t*>(alloc.pMem_);

	for(uint32_t i=0; i<nThreadNum; ++i)
		anAffine[i] = i % app::cpu_logic_core();

	pWorker_->kick(nThreadNum, anAffine);
}

void resource_manager::init(const shared_ptr<concurrent::worker>& pWorker)
{
	pWorker_ = pWorker;
}

/////////////////////////////

bool resource_manager::add_resource_info(const string_fw& sID, const string& sFile, bool bTextMode)
{
	tuple<string,bool> tFile(sFile,bTextMode);
	auto r = mapID_.emplace(sID, tFile);
	if(!r.second)
	{
		logger::warnln("[resource_manager]IDがすでに登録されています。: " + sID.get());
		return false;
	}

	return true;
}

void resource_manager::remove_resource_info(const string_fw& sID)
{
	mapID_.erase(sID);
}

resource_manager::request_result resource_manager::request(const string_fw& sID)
{
	auto cache = cacheResouce_.find(sID);
	if(cache!=cacheResouce_.end())
	{// キャッシュ上に存在している
		auto f = cache->second.get<0>().lock();
		if(!f || f->is_fin())
		{
			uint32_t n = cache->second.get<1>()->state();
			if(n==FAIL || n==SUCCESS) return EXIST;
		}
		
		return LOADING;
	}

	auto id = mapID_.find(sID);
	if(id==mapID_.end())
	{
		logger::warnln("[resource_manager]IDとファイル名が登録されていません。: " + sID.get());
		return FAIL;
	}

	
	auto file = make_shared<resource_file>();
	file->set_filepath(id->second.get<0>(), id->second.get<1>());

	resource_type res;
	res.get<1>() = file;
	res.get<0>() = pWorker_->request([file](){ (*file)(); });

	cacheResouce_.emplace(sID, res);

	return SUCCESS;
}

optional<shared_ptr<resource_file>> resource_manager::resource(const string_fw& sID)
{
	auto cache = cacheResouce_.find(sID);
	if(cache!=cacheResouce_.end())
	{// キャッシュ上に存在している
		auto f = cache->second.get<0>().lock();
		if(f==nullptr || f->is_fin())
		{
			uint32_t n = cache->second.get<1>()->state();
			if(n==resource_file::FAIL || n==resource_file::SUCCESS) 
				return optional<shared_ptr<resource_file>>(cache->second.get<1>());
		}
	}
	return optional<shared_ptr<resource_file>>();
}

void resource_manager::release_cache(const string_fw& sID)
{
	cacheResouce_.erase(sID);
}

const string& resource_manager::filename(const string_fw& sID)
{
	auto id = mapID_.find(sID);
	if(id==mapID_.end())
	{
		logger::warnln("[resource_manager]IDとファイル名が登録されていません。: " + sID.get());
		return std::move(string());
	}

	return id->second.get<0>();
}

bool resource_manager::load_resource_info(const string& sFilePath, bool bFile)
{/*
<resource_def>
	<file id="" src="" text="true/false" />
</resource_def>
 */

	std::stringstream ss;
	if(!file::load_file_to_string(ss, sFilePath, bFile))
	{
		logger::warnln("[resource_manager]リソース情報ファイルが読み込めませんでした。: " + sFilePath);
		return false;
	}

	using namespace p_tree;
	ptree xml;
	xml_parser::read_xml(ss, xml, xml_parser::no_comments);

	optional<ptree&> def = xml.get_child_optional("resource_def");
	if(!def)
	{
		logger::warnln("[resource_manager]リソース定義ファイルのフォーマットが間違っています。");
		return false;
	}

	uint32_t n=1;
	for(auto& it : *def)
	{
		if(it.first=="file")
		{
			string sID		= it.second.get<string>("<xmlattr>.id","");
			string sSrc		= it.second.get<string>("<xmlattr>.src","");
			
			if(sID.empty() || sSrc.empty())
			{
				logger::warnln("[resource_manager]" + to_str_s(n) + "個目のタグのフォーマットが間違っています。");
				return false;
			}

			bool   bText	= it.second.get<bool>("<xmlattr>.text",false);

			add_resource_info(string_fw(sID), sSrc, bText);
		}

		++n;
	}

	return true;
}


} // namespace mana end
