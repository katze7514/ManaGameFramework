#pragma once

namespace mana{
namespace app{

//! @brief アプリケーションイニシャライザ
/*! 二重起動防止や、システム情報取得などを行う */
class app_initializer
{
public:
	app_initializer();
	~app_initializer();

	//! @param[in] sMultiBootBan 多重起動禁止に使う文字列。空文字の時は多重起動禁止しない
	//! @param[in] bAeroDisEnable Aeroを切るならtrue
	//! @return falseが返って来たら、すでに起動してるので、アプリを終了させること
	bool init(const string& sMultiBootBan="", bool bAeroDisEnable=true);

private:
	HANDLE hMutex_;
};

} // namespace app end
} // namespace mana end
