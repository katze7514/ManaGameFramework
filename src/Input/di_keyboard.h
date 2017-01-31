#pragma once

namespace mana{
namespace input{
// 前方宣言
class di_driver;

//! 物理キーボード入力を取得する（DirectInput実装）
class di_keyboard
{
public:
	di_keyboard():pDevice_(nullptr),curIndex_(0),bAcquire_(false),bGuard_(false){ buf_clear(); }
	~di_keyboard(){ fin(); }

	//! @brief 初期化
	/*! @param[in] adaptar DirectInputアダプターインスタンス
	    @param[in] hWnd キーボード入力を受け付けるウインドウハンドル
		@param[in] bBackGraound trueだと非アクティブでもキー入力を受け付けるようになる */
	bool init(di_driver& adapter, HWND hWnd, bool bBackGround=false);

	//! 終了処理
	void fin();

	//! キーボード入力を取得
	bool input();

	//! キーが押されている
	bool is_press(uint32_t nKey)const;
	//! キーが押された瞬間
	bool is_push(uint32_t nKey)const;
	//! キーが離れた瞬間
	bool is_release(uint32_t nKey)const;

	// 入力ガード
	void guard(bool bGuard);
	bool is_guard()const{ return bGuard_; }

	//! バッファをクリアする
	void buf_clear(){ curIndex_=0; ::ZeroMemory(keyBuf_, sizeof(keyBuf_)); }

	//! 現在のバッファをクリアする
	void curbuf_clear(){ ::ZeroMemory(keyBuf_[curIndex_], 256*sizeof(BYTE)); }

private:
	//! キーボード用DirectInputデバイス
	LPDIRECTINPUTDEVICE8 pDevice_;

	//! キーバッファ
	BYTE keyBuf_[2][256];
	//! キーバッファの履歴位置
	uint32_t curIndex_;

	//! デバイスを獲得しているかどうか
	bool bAcquire_;

	//! @brief 入力ガードフラグ
	/*! trueになすると入力バッファがクリアされ、inputを呼んでも更新されない
	 *  つまり、入力されてない扱いになる */
	bool bGuard_;
};

typedef shared_ptr<di_keyboard> di_keyboard_ptr;

} // namespace input end
} // namespace mana end
