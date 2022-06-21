# dromozoa-web

## とりあえずの目標

-[ ] LuaをビルドしてWASMを生成する
  -[x] emscriptenを導入する
  -[x] Luaをレポジトリに追加する
  -[ ] LuaをCでコンパイルしてWASMを生成する
  -[ ] LuaをC++でコンパイルしてWASMを生成する
-[ ] Luaとお役立ちモジュールをビルドしてWASMを生成する
  -[ ] 生メモリを直接扱う
  -[ ] JavaScript関数を扱う
  -[ ] コルーチンはどうしよう

## おぼえがき

### emscriptenの準備

参考: https://emscripten.org/docs/tools_reference/emsdk.html

```sh
cd /opt
sudo mkdir -p emsdk
sudo chown `id -u`: emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk --help
./emsdk list
./emsdk install latest
./emsdk activate latest
```

1.1GBあった。

### emscriptenを使う設定

使うときだけインポートする。

```
. /opt/emsdk/emsdk_env.sh
```

