# 88VA Eternal Grafx ビルド方法

## ビルド確認環境

* Windows 10 Pro 21H2
* Visual C++ 2005 Express Edition
* nasm 0.98.39
* Windows Vista Software Development Kit (SDK)
* DirectX SDK February 2010

### ツールの入手

* Visual C++ 2005 Express Edition
    * Microsoft の Visual Studio Dev Essentials に登録して無料でダウンロードできる。名称は "Visual Studio Express 2005 for C++" となっている。
* nasm
    * https://sourceforge.net/projects/nasm/files/Win32%20binaries/
    * ファイル名が `nasmw.exe` だった時期の旧版を利用。
* Windows Vista Software Development Kit (SDK)
    * Microsoft の Visual Studio Dev Essentials に登録して無料でダウンロードできる。
* DirectX SDK February 2010
    * 現時点での入手手段なし。
    * Microsoft の Visual Studio の有償のサブスクリプションでは、DirectX Software Development Kit (SDK) - November 2007 まで配布されているが、それも利用できるはず。
    * なお、DirectX SDK June 2010 は無償で入手できるが ddraw.lib が削除されているため、利用不可。

### ツールのインストール

* Visual C++ 2005 Express Edition
    * インストール時に .NET Framework 3.5 のインストールが要求されるため、それに従う。
    * インストールがエラー終了するが (.NET Framework 2.0 のインストール失敗が原因)、再度実行する。
    * インストール完了後もVC++を起動すると「このアプリは正しく動作しない可能性があります」のメッセージが表示されるが無視する。
* nasm
    * `C:\bin\nasm` に `nasmw.exe` を置く。プロジェクトの設定で直接このパスを指定してしまっているため。
* Windows SDK
    * FxCop を除外してインストール。インストールに失敗するため。

### VC++ の設定

* ツール > オプション > VC++ディレクトリ
    * インクルードファイルに以下を追加。
        * `<Windows SDK のインストールパス>\Include`
        * `<DirectX SDK のインストールパス>\Include`
    * ライブラリファイルに以下を追加。
        * `<Windows SDK のインストールパス>\Lib`
        * `<DirectX SDK のインストールパス>\Lib\x86`

### ビルド

* VC++ で `Win9x\np2.sln` を開く。
* ビルド > 構成マネージャー で、「アクティブソリューション構成」欄で Debug または Release を選択。
* ビルド > ソリューションのビルド


