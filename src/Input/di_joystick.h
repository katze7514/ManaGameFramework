#pragma once

namespace mana{
namespace input{
// 前方宣言
class di_driver;

//! joystick_infoで使うデバイスインスタンスのオプション型
typedef optional<const DIDEVICEINSTANCE&> device_inst_opt;

/*! @brief 使用可能なジョイスティックを管理するクラス
 *
 *	使用可能なジョイスティックの情報を集めておき、必要に応じて
 *	その情報を返す
 */
class di_joystick_enum
{
public:
	di_joystick_enum(){  vecJoyStickInfo_.reserve(8); }

	bool init(di_driver& adapter);

	//! 使用可能なジョイスティックの数
	uint32_t enabled_joystick_num()const{ return vecJoyStickInfo_.size(); }

	//! @brief 使用可能なジョイスティックの情報を取得する
	/*! @param[in] nJoyNo 情報を取得するジョイスティック番号。0以上enable_joystick_num()以下 
	 *  @return ジョイスティック情報のoptioanl。ジョイスティックが存在していれば該当の値が入っている */
	device_inst_opt joystick_info(uint32_t nJoyNo)const;

private:
	//! DirectInput::EnumDeviceに渡すコールバック関数
	static BOOL CALLBACK on_enum_device(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);

private:
	//! ジョイスティック情報配列
	vector<DIDEVICEINSTANCE> vecJoyStickInfo_;

	NON_COPIABLE(di_joystick_enum);
};

/*! @brief ジョイスティック一つを表すクラス
 *
 *	di_joystick_enumから、取得した情報を元にデバイスを作成し
 *	実際に入力処理を行う
 *  軸・回転の範囲は、-1.0～1.0 を取る。あそびの範囲にある時は0.0が返る
 *
 *  AD変換をONにすると、XY軸デジタルがPOVとして、POVがXY軸デジタル入力としても扱われる
 *  XY軸がis_pov系メソッドで、POV系がis_axis系メソッドで取得できるようになる
 */
class di_joystick
{
public:
	static const int32_t MAX_AXIS_VALUE;

	//! 方向を示す列挙子
	enum arrow : uint32_t
	{
		// 通常のボタンの次からの番号を振る
		POV_NONE=32,	//!< POV何も押されていない
		POV_UP,			//!< POV↑
		POV_RIGHT,		//!< POV→
		POV_DOWN,		//!< POV↓
		POV_LEFT,		//!< POV←
		AXIS_NONE,		//!< XY軸何も押されていない
		AXIS_UP,		//!< 軸Y↑
		AXIS_RIGHT,		//!< 軸X→
		AXIS_DOWN,		//!< 軸Y↓
		AXIS_LEFT,		//!< 軸X←
		ARROW_END,
	};


public:
	di_joystick():pDevice_(nullptr),nCurIndex_(0),bAcquire_(false),
				  bADConv_(true),
				  nX_Threshold_(300),nY_Threshold_(300),nZ_Threshold_(300),
				  nXrot_Threshold_(300),nYrot_Threshold_(300),nZrot_Threshold_(300),
				  bGuard_(false){ buf_clear(); }

	~di_joystick(){ fin(); }

	//! @brief 初期化
	/*! @param[in] joyins デバイスを作成するデバイスインスタンス
		@param[in] adaptar DirectInputアダプターインスタンス
	    @param[in] hWnd キーボード入力を受け付けるウインドウハンドル
		@param[in] bBackGraound trueだと非アクティブでもキー入力を受け付けるようになる */
	bool init(const DIDEVICEINSTANCE& joyins, di_driver& adapter, HWND hWnd, bool bBackGround=false);

	//! 終了処理
	void fin();

	//! ジョイスティック入力を取得
	bool input();

	//! AD変換が有効か
	bool is_ad_conv()const{ return bADConv_; }
	//! AD変換フラグ設定
	bool ad_conv(bool bAD){ bADConv_ = bAD; }

	//! アナログ軸のあそびの閾値設定。この値以下の入力は無効扱い
	void set_axis_threshold(uint32_t nX, uint32_t nY, uint32_t nZ){ nX_Threshold_=nX; nY_Threshold_=nY; nZ_Threshold_=nZ; }
	//! アナログ回転のあそびの閾値設定。この値以下の入力は無効扱い
	void set_rot_threshold(uint32_t nX, uint32_t nY, uint32_t nZ){ nXrot_Threshold_=nX; nYrot_Threshold_=nY; nZrot_Threshold_=nZ; }

	//! x軸の値
	float	x()const;
	//! y軸の値
	float	y()const;
	//! z軸の値
	float	z()const;

	//! x軸の1つ前の値
	float	x_prev()const;
	//! y軸の1つ前の値
	float	y_prev()const;
	//! z軸の1つ前の値
	float	z_prev()const;

	//! x回転の値
	float	x_rot()const;
	//! y回転の値
	float	y_rot()const;
	//! z回転の値
	float	z_rot()const;

	//! x回転の1つ前の値
	float	x_rot_prev()const;
	//! y回転の1つ前の値
	float	y_rot_prev()const;
	//! z回転の1つ前の値
	float	z_rot_prev()const;

	//! pov値
	int32_t pov()const;
	//! povの1つ前の値
	int32_t pov_prev()const;

	//! ボタンが押されている。0～31はボタン。32～はPOVと軸
	bool is_press(uint32_t nBtn)const;
	//! ボタンが押された瞬間。0～31はボタン。32～はPOVと軸
	bool is_push(uint32_t nBtn)const;
	//! ボタンが離れた瞬間。0～31はボタン。32～はPOVと軸
	bool is_release(uint32_t nBtn)const;

	// 入力ガード
	void guard(bool bGuard);
	bool is_guard()const{ return bGuard_; }

	//! バッファをクリアする
	void buf_clear(){ nCurIndex_=0; ::ZeroMemory(joyBuf_, sizeof(joyBuf_)); }

	//! 現在のバッファをクリアする
	void curbuf_clear(){ ::ZeroMemory(&joyBuf_[nCurIndex_], sizeof(DIJOYSTATE)); }

private:
	//! POVが押されている
	bool is_pov_press(enum arrow eArrow)const;
	//! POVが押された瞬間
	bool is_pov_push(enum arrow eArrow)const;
	//! POVが離れた瞬間
	bool is_pov_release(enum arrow eArrow)const;


	//! POVが押されているかどうかのチェック
	/*! @param[in] eArrow チェックする方向
		@param[in] index チェックするバッファインデックス */
	bool is_pov_press_inner(enum arrow eArrow, uint32_t nIndex)const;

	//! XY軸がデジタル的に押されている
	bool is_axis_press(enum arrow eArrow)const;
	//! XY軸がデジタル的に押された瞬間
	bool is_axis_push(enum arrow eArrow)const;
	//! XY軸がデジタル的に離れた瞬間
	bool is_axis_release(enum arrow eArrow)const;

	//! XY軸がデジタル的に押されているかどうかのチェック
	/*! @param[in] eArrow チェックする方向
		@param[in] index チェックするバッファインデックス */
	bool is_axis_press_inner(enum arrow eArrow, uint32_t nIndex)const;

	//! POV_*とAXIS_* を相互変換する
	enum arrow swap_pov_axis(enum arrow eArrow)const;

private:
	//! DirectInputDevice::EnumObjectsに渡すコールバック関数
	static BOOL CALLBACK on_enum_axis(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);

private:
	//! デバイス
	LPDIRECTINPUTDEVICE8	pDevice_;

	//! ジョイスティック入力バッファ
	DIJOYSTATE	joyBuf_[2];
	//! ジョイスティックバッファ位置
	uint32_t	nCurIndex_;

	//! デバイスを獲得しているかどうか
	bool		bAcquire_;

	//! アナログX,YをPOV(十字キー)として見なすかどうか 
	bool		bADConv_;

	// joystickのアナログの遊びの値　-threshold～threshold の範囲になる
	// これ以下の値の時は入力なし扱い

	uint32_t	nX_Threshold_;
	uint32_t	nY_Threshold_;
	uint32_t	nZ_Threshold_;

	uint32_t	nXrot_Threshold_;
	uint32_t	nYrot_Threshold_;
	uint32_t	nZrot_Threshold_;

	//! @brief 入力ガードフラグ
	/*! trueになすると入力バッファがクリアされ、inputを呼んでも更新されない
	 *  つまり、入力されてない扱いになる */
	bool bGuard_;
};

typedef shared_ptr<di_joystick> di_joystick_ptr;


/*! @brief di_joystickを作成管理するクラス
 *
 *  di_joystick_enum/di_joystickをまとめる。
 *  Joystickを取得する時はこれを使うと良い
 */
class di_joystick_factory
{
public:
	typedef flat_map<uint32_t, di_joystick_ptr> joy_map;

	di_joystick_factory(){}
	~di_joystick_factory(){ fin(); }

	bool init(HWND hWnd, const shared_ptr<di_driver>& pDriver, bool bBackGround=false);
	void fin();

	//! 使用可能なジョイスティックの数
	uint32_t enabled_joystick_num()const{ return joyEnum_.enabled_joystick_num(); }

	//! @brief 指定の番号のdi_joystickを取得する
	/*! @return すでに存在したらそれを返す。存在しないjoystickだったら生成する。生成が失敗したらnullptrが返る */
	di_joystick_ptr joystick(uint32_t nNo);

private:
	shared_ptr<di_driver>	pAdapter_;
	HWND					hWnd_;
	bool					bBackGround_;

	di_joystick_enum	joyEnum_;
	joy_map				mapJoy_;

private:
	NON_COPIABLE(di_joystick_factory);
};

} // namespace input end
} // namespace mana end

/*	使用例
   di_joystick_enumで接続されているジョイスティックの情報を集め、
   その集めた情報を使って、di_joystickを初期化して、入力を取得する
 
 	di_driver di;
 	di.init();
 
 	di_joystick_enum jenum;
 	jenum.init(di);
 
 	di_joystick joy;
 	device_inst_opt info = jenum.joystick_info(0); // 0番目のジョイスティック上方を取得
 	if(info) joy.init(*info, di, wnd_handle());

	もしくは、di_joystick_factoryを使って

	di_driver di;
 	di.init();

	di_joystick_factory input_factory(di);
	input_factory.init(wnd_handle(),false);

	di_joystick_ptr joy0 = input_factory.get_joystick(0);

	と使う
 */
