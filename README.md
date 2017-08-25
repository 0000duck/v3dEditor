# v3dEditor
___

Vulkan のラッパーライブラリである v3d のデモンストレーションとして作成したモデルエディタです。  
  
![top](https://user-images.githubusercontent.com/28208742/29695231-8a12ec46-897c-11e7-9099-51df44fdc178.png)
    
## はじめに
  
### 必須環境
- OS : Microsoft Windows 10 32Bit または 64Bit
- システムメモリ : 4GB 以上
- ビデオカード : Vulkan に対応していて、ビデオメモリが 2GB 以上 のもの

### 推奨環境
- OS : Microsoft Windows 10 64Bit
- システムメモリ : 8GB 以上
- ビデオカード : Vulkan に対応していて、ビデオメモリが 4GB 以上 のもの

### インストールしておく必要があるもの
- Microsoft Visual Studio 2017  
- FBX SDK 2017.0.1 VS2015  
- Vulkan SDK 1.0.57.0  
  
### レポジトリの準備
git clone --recursive https://github.com/mixberry8/v3dEditor.git  
git submodule init  
git submodule update  
  
### ビルド
external/v3d/build/runtime/solutions/vs2017/v3d.sln を開き、Release_Static をビルドします。

v3dEditor.sln を開き、Release をビルドします。  
v3dEditor のルートディレクトリに bin フォルダが作成されその中に、本体である "v3dEditor.exe" が配置されます。  
また同時に v3dEditor が使用する "shaderCompiler.dll" も配置されます。
  
## 操作方法
- カメラの回転 : マウスの中ボタンを押しながらドラッグ
- カメラの移動 : Shift + マウスの中ボタンを押しながらドラッグ
- カメラの距離 : Ctrl + マウスの中ボタンを押しながら上下にドラッグ または マウスホイール
- メッシュの選択および解除 : マウスの右クリック
  
## 主なインターフェース

### MainMenu
  
![mainmenu](https://user-images.githubusercontent.com/28208742/29695208-6457f33e-897c-11e7-87ca-138d88af7231.png)
  
- File
  - New : 新規にモデルを作成します。  
  - Open : 既存のモデルを読み込みます。  
  指定するファイルの拡張子は xml になります。  
  ※ 詳細は Save を参照してください。
  - Save : 現在編集中のモデルを保存します。  
  インポートしたモデルのディレクトリに同じファイル名で、拡張子が skm 及び xml のファイルを作成します。
    - skm : v3dEditor 専用フォーマットのモデルファイルです。
    - xml : シーンの設定を記録しているファイルです。  
  既存のモデルを読み込む際は、このファイルを指定することになります。
  - Quit : アプリケーションを終了します。
- Display 
  - LightShape : ライトの形状を表示します。
  - MeshBounds : メッシュの境界を表示します。
- Window
  - Fps : 毎秒のフレーム数を表示します。
  - Outliner : アウトライナーを表示します。
  - Inspector : インスペクターを表示します。
  - Log : ログを表示します。
- Help
  - About : アプリケーションの情報を表示します。

### New  
新規にモデルをインポートします。  
  
![new](https://user-images.githubusercontent.com/28208742/29695217-7063cc2a-897c-11e7-8b4c-1e69df5e58ef.png)
1. インポートするモデルファイルです。  
現在インポートできるモデルファイルは FBX のみになります。  
2. ポリゴンの面を反転します。
3. テクスチャの U 座標を反転します。
4. テクスチャの V 座標を反転します。
5. 法線を反転します。  
トラブルが発生しない限りチェックを入れることな無いと思われます。  
6. モデルの回転およびスケールを設定します。
7. モデルの最適化を行います。
8. モデルのスムージングを行います。  
またスムージングは モデルの最適化が有効でないと機能しません。
9. モデルが参照するファイル ( テクスチャなど ) のパスタイプ指定します。  
Relative 相対パスで参照します。  
Strip ファイルパスに含まれるディレクトリを取り除き、ファイル名だけで参照します。  

### Outliner
シーン及びワールドに配置されているオブジェクト一覧を表示します。  
  
![outliner](https://user-images.githubusercontent.com/28208742/29695223-7d03c246-897c-11e7-8bca-bfaed472727d.png)
  
ここで選択されているものが、Inspector で編集することができます。  
またメッシュはスクリーンを右クリックすることによって選択することも可能です。  

### Inspector
シーン及びワールドに配置されているカメラ、ライト、モデルの編集をします。
  
#### Common
ローカルトランスフォームの編集および表示、ワールドトランスフォームを表示します。  
  
![inspector_common](https://user-images.githubusercontent.com/28208742/29695153-173d1a02-897c-11e7-9a95-feb54fb32607.png)
- Local : ローカルトランスフォームです。  
この値は変更することができます。
- World : ワールドトランスフォームです。  
  
#### Scene
グリッド、アンビエントオクルージョン ( SSAO )、シャドウ、グレア、イメージエフェクト、その他カラーの編集をします。  
  
![inspector_scene](https://user-images.githubusercontent.com/28208742/29695201-57f41500-897c-11e7-8ad6-ba51a90551d9.png)
- Grid
  - CellSize : セルのサイズです。
  - HalfCellCount : セルの数です。  
  一方向に向かって描画されるセルの数は HalfCellCount * 2 になります。
  - Color : グリッドの色です。
- AmbientOcclusion
  - Radius : サンプリングする範囲です。
  - Threshold : 遮断の判定を開始する深度のオフセットです。
  - Depth : 遮断されていると判断する深さです。  
  - Intensity : 影の濃さです。
  - BlurRadius : ブラーリングの範囲です。  
  この値が大きいほどより強くぼかしがかかります。
  - BlurSamples : ブラーリングのサンプル数です。  
  この値が大きいほどきめ細かいぼかしがかかります。
- Shadow
  - Resolution : シャドウ画像の解像度です。  
  この値が大きいほど影が鮮明になります。
  - Range : 影を落とす範囲です。  
  この値が大きいほど広範囲のオブジェクトが影を落としますが、影があらくなります。
  - FadeMargin : 遠くの影がフェードアウトを始める範囲です。  
  影がフェードアウトを開始する位置は CenterOffset + Range - FadeMargin になります。
  - Dencity : 影の濃さです。
  - CenterOffset : 影を落とす範囲 ( 球 ) の中心の視線方向のオフセットです。  
  このオフセットを奥 ( + ) にした際は後方の遠くのオブジェクトの影が描画されなくなり、  
  また手前 ( - ) にした場合は前方の遠くのオブジェクトの影が描画されなくなります。
  - BoundBias : ライトから遠いオブジェクトの影が描画されていない場合は、この値を大きくしてください。
  - DepthBias : 影を落とされると判断する深度のオフセットです。
- Glare
  - BrightThreshold : この値が大きいほど暗いと思われる部分がカットされます。
  - BrightOffset : この値が小さいほど抜き出された明るい部分が強調されます。
  - BlurRadius : ブラーリングの範囲です。  
  この値が大きいほどより強くぼかしがかかります。
  - BlurSamples : ブラーリングのサンプル数です。  
  この値が大きいほどきめ細かいぼかしがかかります。
  - Intensity1 : １枚目のグレア画像を合成する際の明るさです。
  - Intensity2 : ２枚目のグレア画像を合成する際の明るさです。  
  １枚目のグレア画像に比べて、２枚目のグレア画像は倍大きく、強くぼかしがかかっています。
- ImageEffect
  - ToneMapping : トーンマッピングをします。
  - Fxaa : Fxaa を使用してアンチエイリアスをかけます。
- Misc
  - BackgroundColor : 背景色です。
  - SelectedColor : メッシュを選択した際の輪郭の色です。
  - MeshBoundsColor : カメラのフラスタムに入っているかどうかを判定する境界の色です。
  - LightShapeColor : ライト形状の色です。
  
#### Camera
カメラの編集を行います。  
  
![inspector_camera](https://user-images.githubusercontent.com/28208742/29695126-efb1c6d6-897b-11e7-9833-a6a39c264343.png)
- FovY 画角です。
- FarClip ファークリップです。  
この値が大きいほどより遠くのオブジェクトが表示されるようになります。  
  
#### Light
ライトの編集をします。  
  
![inspector_light](https://user-images.githubusercontent.com/28208742/29695178-3ccefcfe-897c-11e7-94c7-9ddabe973d6a.png)
- Color ライトの色です。
- Rotation ライトの向きをマウスのドラッグで編集します。  
表示されている角度の値はローカルトランスフォームと同じになります。  
  
#### Mesh
メッシュの編集をします。  
  
![inspector_mesh](https://user-images.githubusercontent.com/28208742/29695186-48e8cf56-897c-11e7-8053-448b2869f0f0.png)
- Mesh
  - Polygons : ポリゴン数です。
  - Bones : ボーン数です。
- Material
  - DiffuseColor : 拡散反射の色です。
  - DiffuseFactor : 拡散反射の係数です。  
  この値が小さいほど暗くなります。
  - EmissiveFactor : 発光の係数です。  
  この値が大きいほどより強く発光します。
  - SpecularColor : 鏡面反射の色です。
  - Shiness : 鏡面反射のハイライトの強さです。
  - DiffuseTexture : 拡散反射のテクスチャです。※1
  - SpecularTexture : 鏡面反射のテクスチャです。※1,2
  - BumpTexture : 法線のテクスチャです。※1  
  RGB に法線を格納している必要があります。
  - TextureFilter : テクスチャのフィルタです。
    - Nearest : フィルタリングをしません。
	- Linear : 線形補完をします。
  - TextureAddressMode : テクスチャのアドレスモードです。
    - Repeat : テクスチャを繰り返します。
	- MirroredRepeat : テクスチャを反転して繰り返します。
	- ClampToEdge : テクスチャのエッジを繰り返します。
  - BlendMode : ブレンドモードです。
    - Copy : コピーします。
    - Normal : アルファブレンディングをします。
    - Add : 加算合成をします。
    - Sub : 減算合成をします。
    - Mul : 乗算合成をします。
    - Screen : スクリーン合成をします。
  - CullMode : カリングのモードです。
    - None : カリングをしません。
	- Front : 前面のポリゴンをカリングします。
	- Back : 背面のポリゴンをカリングします。  
  
※1 使用できるテクスチャのフォーマットは DDS KTX のみになります。  
※2 色は反映されずシェーダーでは輝度として処理しています。

## サンプル
examples ディレクトリに [Crytek's Atrium Sponza Palace](http://www.crytek.com/cryengine/cryengine3/downloads) を FBX 形式に変換したものと、
最適化済みの SKM ファイル 及び XML ファイルが入っています。  
  
## 既存の問題
- 半透明のメッシュの描画がいいかげんなため、非常に重たいです。  
  ポリゴン数の多いものは、半透明描画は避けてください。  
- デバイスメモリがカツカツな状態でウィンドウのサイズを大きくすると、デバイスメモリが不足してアプリケーションが強制終了することがあります。  
  モデルを読み込む前にウィンドウのサイズを変更しておくことで発生しなくなる場合があります。  
- ウィンドウのサイズの変更が非常に重たいです。  
  気にしないでください、、、
- 異なるモデルを読み込んだ際にアプリケーションが落ちる場合があるかもしれません。  
  たぶん治ってる、、、
- たまに ImGui の CollapsingHeader の逆三角形のマークをクリックしても開閉できなくなる場合があります。  
  原因はよくわかりません、、、
  
## ライセンス
MIT
