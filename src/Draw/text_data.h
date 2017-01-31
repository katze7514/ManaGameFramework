#pragma once

namespace mana{
namespace draw{

class text_data
{
public:
	text_data():bMarkUp_(false){}
	text_data(const string_fw& sFontID, const string& sText, bool bMarkUp):sFontID_(sFontID),sText_(sText),bMarkUp_(bMarkUp){}

public:
	const string_fw&	font_id()const{ return sFontID_; }
	void				set_font_id(const string_fw& sFontID){ sFontID_=sFontID; }

	const string&		text(){ return sText_; }
	const string&		text()const{ return sText_; }
	text_data&			set_text(const string& sText){ sText_=sText; return *this; }
	text_data&			add_text(const string& sText){ sText_+=sText; return *this; }

	bool				is_markup()const{ return bMarkUp_; }
	text_data&			markup(bool bMarkUp){ bMarkUp_=bMarkUp; return *this;}

private:
	string_fw	sFontID_; // フォントID。マークアップされた場合はそちらに従う
	string		sText_;
	bool		bMarkUp_; // trueだと装飾されたテキスト
};

} // namespace draw end
} // namespace mana end
