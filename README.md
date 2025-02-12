# 音声劣化 AviUtl 拡張編集フィルタプラグイン

AviUtl の拡張編集に「音声劣化」フィルタ効果とフィルタオブジェクトを追加するプラグインです．音声のサンプルレートやビットレートを疑似的に下げて，音質を意図的に落とします．

![劣化の例](https://github.com/user-attachments/assets/e5e3d35c-4689-41ff-9db7-2c19701331c5)

https://github.com/user-attachments/assets/725bccd1-326c-4511-8ad2-9d9d3b8ac0ea


## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl
  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

- patch.aul の `r43 謎さうなフォーク版58` (`r43_ss_58`) 以降

  https://github.com/nazonoSAUNA/patch.aul/releases/latest


## 導入方法

`aviutl.exe` と同階層にある `plugins` フォルダ内に `AudioDecay.eef` ファイルをコピーしてください．

## 使い方

音声系オブジェクトのフィルタ効果の追加メニューとフィルタオブジェクトの追加メニューから「音声劣化」でフィルタを追加します．

![GUI表示](https://github.com/user-attachments/assets/6fd82f59-757d-4683-9afc-1908bfa2954d)

- `サンプルHz` で劣化目標のサンプリングレートを Hz 単位で指定します．

  - スライダーの移動範囲は `8000` から `48000` までですが，直接数値入力（あるいは数値部分を左右にドラッグ移動）で `40` から `96000` まで指定できます．初期値は `48000`.

  - プロジェクトの音声サンプリングレート以上に増やしても音質は向上しません (劣化処理もされません).

- `ビット深度` で劣化目標の音声 1 サンプル当たりのビット数を指定します．

  - 疑似的に小数点以下 2 桁まで指定できます．その場合，サンプルの量子化幅が連続的に変化します．

  - スライダーの移動範囲は `8.00` から `16.00` までですが，直接数値入力（あるいは数値部分を左右にドラッグ移動）で最小値 `1.00` まで下げられます．初期値は `16.00` (無劣化).

  - AviUtl の取り扱う音声のビット深度は `16` ビットで固定です．

- `ディザリング` で `ビット深度` による劣化計算時に[誤差拡散](https://e-words.jp/w/%E3%83%87%E3%82%A3%E3%82%B6%E3%83%AA%E3%83%B3%E3%82%B0.html)を適用します．

  - `ビット深度` が最大値の `16.00` の場合は無視されます．

  - 初期値は ON.


## TIPS

- あまり強く劣化させると音割れのようになることがあるため注意してください．


## 改版履歴

- **v1.00** (2025-02-12)

  - 初版．


## ライセンス・免責事項

このプログラムの利用・改変・再頒布等に関しては CC0 として扱うものとします．


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk （利用したブランチは[こちら](https://github.com/sigma-axis/aviutl_exedit_sdk/tree/self-use)です．）

---

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# 連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://x.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
