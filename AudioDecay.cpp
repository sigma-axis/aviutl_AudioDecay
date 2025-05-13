#include <cstdint>
#include <algorithm>
#include <vector>
#include <numeric>
#include <cmath>
#include <numbers>
#include <bit>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using byte = uint8_t;
#include <exedit.hpp>

////////////////////////////////
// 主要情報源の変数アドレス．
////////////////////////////////
constinit struct ExEdit092 {
	void init(AviUtl::FilterPlugin* efp)
	{
		if (fp == nullptr)
			init_pointers(efp);
	}
	AviUtl::FilterPlugin* fp = nullptr;

	// called at: exedit_base + 0x1c1ea.
	void* (*get_or_create_cache)(ExEdit::ObjectFilterIndex ofi, int w, int h, int bitcount, int v_func_id, int* old_cache_exists);

private:
	void init_pointers(AviUtl::FilterPlugin* efp)
	{
		fp = efp;

		auto pick_addr = [exedit_base = reinterpret_cast<intptr_t>(efp->dll_hinst), this]
			<class T>(T & target, ptrdiff_t offset) { target = reinterpret_cast<T>(exedit_base + offset); };
		auto pick_call_addr = [&]<class T>(T & target, ptrdiff_t offset) {
			ptrdiff_t* tmp; pick_addr(tmp, offset);
			target = reinterpret_cast<T>(4 + reinterpret_cast<intptr_t>(tmp) + *tmp);
		};

		pick_call_addr(get_or_create_cache, 0x01c1ea);
	}
} exedit{};


////////////////////////////////
// 仕様書．
////////////////////////////////
struct check_data {
	enum def : int32_t {
		unchecked = 0,
		checked = 1,
		button = -1,
		dropdown = -2,
	};
};

#define PLUGIN_VERSION	"v1.02-beta1"
#define PLUGIN_AUTHOR	"sigma-axis"
#define FILTER_INFO_FMT(name, ver, author)	(name##" "##ver##" by "##author)
#define FILTER_INFO(name)	constexpr char filter_name[] = name, info[] = FILTER_INFO_FMT(name, PLUGIN_VERSION, PLUGIN_AUTHOR)
FILTER_INFO("音声劣化");

// trackbars.
constexpr const char* track_names[] = { "サンプルHz", "ビット深度" };
constexpr int32_t
	track_denom[]		= {     1,  100, },
	track_min[]			= {    40,  100, },
	track_min_drag[]	= {  8000,  800, },
	track_default[]		= { 48000, 1600, },
	track_max_drag[]	= { 48000, 1600, },
	track_max[]			= { 96000, 1600, };

static_assert(
	std::size(track_names) == std::size(track_denom) &&
	std::size(track_names) == std::size(track_min) &&
	std::size(track_names) == std::size(track_min_drag) &&
	std::size(track_names) == std::size(track_default) &&
	std::size(track_names) == std::size(track_max_drag) &&
	std::size(track_names) == std::size(track_max));

namespace idx_track
{
	enum id : int {
		sample_rate,
		bit_depth,
	};
};

// checks.
constexpr const char* check_names[] = { "ディザリング", };
constexpr int32_t check_default[] = { check_data::checked, };
static_assert(std::size(check_names) == std::size(check_default));

namespace idx_check
{
	enum id : int {
		dither,
	};
};


BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip);

consteval ExEdit::Filter filter_info(ExEdit::Filter::Flag flag, bool do_init = true) {
	return {
		.flag = flag,
		.name = const_cast<char*>(filter_name),
		.track_n = std::size(track_names),
		.track_name = const_cast<char**>(track_names),
		.track_default = const_cast<int*>(track_default),
		.track_s = const_cast<int*>(track_min),
		.track_e = const_cast<int*>(track_max),
		.check_n = std::size(check_names),
		.check_name = const_cast<char**>(check_names),
		.check_default = const_cast<int*>(check_default),
		.func_proc = &func_proc,
		.func_init = do_init ? [](ExEdit::Filter* efp) { exedit.init(efp->exedit_fp); return TRUE; } : nullptr,
		.information = const_cast<char*>(info),
		.track_scale = const_cast<int*>(track_denom),
		.track_drag_min = const_cast<int*>(track_min_drag),
		.track_drag_max = const_cast<int*>(track_max_drag),
	};
}
inline constinit auto
filter = filter_info(ExEdit::Filter::Flag::Audio, false),
effect = filter_info(ExEdit::Filter::Flag::Audio | ExEdit::Filter::Flag::Effect);


////////////////////////////////
// フィルタ処理．
////////////////////////////////
BOOL func_proc(ExEdit::Filter* efp, ExEdit::FilterProcInfo* efpip)
{
	int constexpr
		min_rate		= track_min[idx_track::sample_rate],
		max_rate		= track_max[idx_track::sample_rate],
		den_depth		= track_denom[idx_track::bit_depth],
		min_depth		= track_min[idx_track::bit_depth],
		max_depth		= track_max[idx_track::bit_depth];

	int const
		raw_rate		= efp->track[idx_track::sample_rate],
		raw_depth		= efp->track[idx_track::bit_depth];

	bool const
		dither			= efp->check[idx_check::dither];

	float
		step_rate = std::max(efpip->audio_rate /
			static_cast<float>(std::clamp(raw_rate, min_rate, max_rate)), 1.0f),
		// note that AviUtl handles only 16-bit depth buffers for audio.
		step_depth = std::max(std::exp2f(16.0f -
			std::clamp(raw_depth, min_depth, max_depth) / static_cast<float>(den_depth)), 1.0f);

	// 過去音声データを保存するキャッシュを取得．
	using i16 = int16_t;
	struct {
		int milliframe;
		float rate_size, rate_remainder;

		// two channels at most.
		int32_t rate_sum[2];
		i16 dither[2];
		i16 data[2];
	}* cache;

	int cache_exists_flag;
	cache = reinterpret_cast<decltype(cache)>(exedit.get_or_create_cache(
		efp->processing, (2 * sizeof(cache[0])) / sizeof(i16), 1, 8 * sizeof(i16),
		0, &cache_exists_flag));

	if (cache == nullptr) return TRUE; // キャッシュを取得できなかった場合何もしない．

	int milliframe = 1000 * (efpip->frame + efpip->add_frame);
	bool is_head = false;
	if (efpip->audio_speed != 0) {
		milliframe = efpip->audio_milliframe - 1000 * (efpip->frame_num - efpip->frame);
		auto const
			speed = 0.000'001 * efpip->audio_speed,
			frame = 0.001 * milliframe;

		step_rate /= static_cast<float>(std::abs(speed));
		if (speed >= 0 ? frame - speed < 0 : frame - speed >= efpip->frame_n) // 想定の前回描画フレームが範囲外．
			is_head = true;
	}
	else if (milliframe == 0) // 1フレーム目
		is_head = true;

	if (is_head || milliframe < cache[0].milliframe || cache_exists_flag == 0)
		// 冒頭部，新規キャッシュ，あるいは時間が巻き戻っているなら初期化．
		std::memset(cache, 0, 2 * sizeof(cache[0]));
	else if (cache[0].milliframe == milliframe)
		// 同一フレームなので音声ソースの更新．読み込み元を巻き戻す．
		cache[0] = cache[1];
	else cache[1] = cache[0];

	auto& state = cache[0];
	if (step_rate > 1) {
		// 規定範囲になるよう修正．
		if (state.rate_size <= 0) state.rate_size = step_rate;
		if (state.rate_remainder <= 0 || state.rate_remainder > state.rate_size)
			state.rate_remainder = state.rate_size;
	}
	else {
		// サンプルレートが無劣化の場合．
		state.rate_size = state.rate_remainder = 1;
		state.rate_sum[0] = state.rate_sum[1] = 0;
	}
	if (!dither || step_depth <= 1) {
		// ディザリングに利用する直前誤差をリセット．
		state.dither[0] = state.dither[1] = 0;
	}
	state.milliframe = milliframe;

	// 音声データの読み書き込み先とサイズを特定．
	i16* const data = efp == &effect ? efpip->audio_data : efpip->audio_p;
	int const num_samples = efpip->audio_n, num_ch = efpip->audio_ch;

	// フィルタ処理．
	for (int i = 0; i < num_samples; i++) {
		i16* const sample = data + i * num_ch;
		if (step_rate > 1) {
			// 平均のための総和計算．
			for (int c = 0; c < num_ch; c++) state.rate_sum[c] += sample[c];

			// 一定数サンプルがたまったら，次の出力値を更新．
			if (--state.rate_remainder <= 0) {
				for (int c = 0; c < num_ch; c++) {
					i16 carry = static_cast<i16>(std::lroundf(sample[c] * (-state.rate_remainder)));
					state.data[c] = static_cast<i16>(std::clamp<int>(std::lroundf(
						(state.rate_sum[c] - carry) / state.rate_size),
						std::numeric_limits<i16>::min(), std::numeric_limits<i16>::max()));
					state.rate_sum[c] = carry;
					if (dither) state.data[c] += state.dither[c];
				}
				state.rate_size = step_rate;
				state.rate_remainder += step_rate;
			}
		}
		else {
			for (int c = 0; c < num_ch; c++) {
				state.data[c] = sample[c];
				if (dither) state.data[c] += state.dither[c];
			}
		}

		// バッファに書き込み．
		if (step_depth > 1) {
			for (int c = 0; c < num_ch; c++) {
				sample[c] = std::clamp<int>(std::lroundf(std::roundf(
					state.data[c] / step_depth) * step_depth),
					std::numeric_limits<i16>::min(), std::numeric_limits<i16>::max());
				if (dither) state.dither[c] = state.data[c] - sample[c];
			}
		}
		else {
			for (int c = 0; c < num_ch; c++)
				sample[c] = state.data[c];
		}
	}

	return TRUE;
}


////////////////////////////////
// 初期化．
////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		::DisableThreadLibraryCalls(hinst);
		break;
	}
	return TRUE;
}


////////////////////////////////
// エントリポイント．
////////////////////////////////
EXTERN_C __declspec(dllexport) ExEdit::Filter* const* __stdcall GetFilterTableList() {
	constexpr static ExEdit::Filter* filter_list[] = {
		&filter,
		&effect,
		nullptr,
	};

	return filter_list;
}
