# dromozoa-web

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

