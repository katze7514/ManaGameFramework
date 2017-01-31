#pragma once

namespace mana{
namespace memory{

// スコープにnew/deleteを閉じ込める
struct scoped_alloc
{
public:
	scoped_alloc(size_t nSize)
	{
		pMem_ = new BYTE[nSize];
		::memset(pMem_, 0 , sizeof(BYTE)*nSize);
	}

	~scoped_alloc()
	{
		delete[] pMem_;
	}

	BYTE* pMem_;
};

} // namespace memory end
} // namespace mana end
