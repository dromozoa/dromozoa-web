# dromozoa-web

## とりあえずの目標

- [ ] LuaをビルドしてWASMを生成する
   - [x] emscriptenを導入する
   - [x] Luaをレポジトリに追加する
   - [x] LuaをCでコンパイルしてWASMを生成する
   - [ ] emscripten runtimeを使う
   - [-] LuaをC++でコンパイルしてWASMを生成する
- [ ] Luaとお役立ちモジュールをビルドしてWASMを生成する
   - [ ] 生メモリを直接扱う
   - [ ] JavaScript関数を扱う
   - [ ] コルーチンはどうしよう
- [ ] どうやってLuaの更新に追随しよう

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

### Luaのビルド方針

* とりあえず、共有ライブラリの動的ロードは行わない
* readlineも使わない
* コルーチンの実装はそのまま
  * Cではlongjmp/setjmp
  * C++ではthrow/try catch

参考: https://emscripten.org/docs/compiling/index.html

```
cd lua
make CC=emcc AR="emar rcu" RANLIB=emranlib MYLDFLAGS="-s STANDALONE_WASM" posix
```

wasmerで動かすと関数の未定義エラーが出た。

```
moyu@arianrhod:/Library/WebServer/Documents/dromozoa-web/lua/src% wasmer run lua.wasm
error: failed to run `lua.wasm`
╰─> 1: Error while importing "env"."getTempRet0": unknown import. Expected Function(FunctionType { params: [], results: [I32] })
```

getTempRet0はlongjmp/setjmpに関係するらしいので、C++でビルドした。

```
cd lua/src
make CC=em++ AR="emar rcu" RANLIB=emranlib MYLDFLAGS="-s STANDALONE_WASM" posix
```

wasmerで動かすと関数の未定義エラーが出た。
wasm2watで怪しいのをさがした。

```
  (import "env" "system" (func (;0;) (type 0)))
  (import "env" "__syscall_dup3" (func (;8;) (type 1)))
  (import "env" "__syscall_unlinkat" (func (;13;) (type 1)))
  (import "env" "__syscall_rmdir" (func (;14;) (type 0)))
  (import "env" "__syscall_renameat" (func (;15;) (type 4)))
```

とりあえず、osモジュールをあきらめてみた。
というか、wasiをあきらめることにした。

```
cd lua/src
make CC=emcc AR="emar rcu" RANLIB=emranlib LUA_T=lua.html posix
```

### wasmerを更新

```
wasmer self-update
```

### httpdの設定

mime.typesでwasmを設定する。


