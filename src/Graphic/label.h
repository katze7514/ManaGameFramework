#pragma once

#include "../Draw/text_data.h"
#include "draw_base.h"

namespace mana{
namespace graphic{

/*! @brief テキストを描画するノード
 *
 *  親までの変換が効くのは描画開始位置とアルファだけ。
 *  拡縮や回転の影響は受けないので注意
 *
 *  ワールド行列計算にも現れないので、テキストノードに
 *　子ノードを追加しても意図した位置に描画されないので特に注意
 */
class label : public draw_base
{
public:
	//! param[in] bCreate falseにするとtext_dataインスタンスを作らない
	label(bool bCreate=true, uint32_t nReserve=CHILD_RESERVE):draw_base(nReserve){ eKind_=DRAW_LABEL; if(bCreate) pTextData_=make_shared<draw::text_data>(); }
	virtual ~label(){}

public:
	const shared_ptr<draw::text_data>&	text_data(){ return pTextData_; }
	label&								set_text_data(const shared_ptr<draw::text_data>& pTextData){ pTextData_=pTextData; return *this; }

	label&								set_text(const string& sText, bool bMarkUp);
	label&								set_font_id(const string_fw& sFont);

protected:
	virtual void	exec_self(draw_context& ctx)override;

protected:
	shared_ptr<draw::text_data>	pTextData_;
};

} // namespace graphic end
} // namespace mana end
