# dromozoa-web

- LuaとJavaScriptで互換性がある型をスタックに乗せてやりとりする
- JavaScript

## 参照

- JavaScript側で強参照をとる
- FinalizationRegistryに登録したら、Lua側のgcを行わないようにする






## 名詞

- referencesを両方につくる
- referenceをつくる
- references
- JavaScript側はFinalizationRegistryでgcを拾う

``` JavaScript
D.refs.get(key);
D.refs.set(key, value);
D.refs.delete(key);
```

``` JavaScript
D.objs.set(key);

D.refs = new FinalizationRegistry(callback);
D.refs.register(obj, ref, ref);
D.refs.unregister(ref);
```

```
event_target:addEventListener(type, listener, use_capture)

-- onceサポート？
[node, type, listener, use_capture]

__gcでnodeが消されたら、イベントリスナのマップも削除する？
→これはダメ
→documentからもdocument_fragmentからもたどれなくなったら？
node.isConnectedでどうにかならない？
→document_fragmentについてるときはisConnectedはfalse
→parentNodeをたどる？
→getRootNode()
→手動で消すようにしたほうがいい気がする

Map.deleteは存在していなくてもエラーにならない。

callback_type関数をつくる
  とりあえず、EventListener？

function => listener

javascriptの例外が出る
  →C++の例外が出る
    →Luaのエラーとして捕捉
      →スタックをまきもどさないと、オブジェクトが解放されない？

JavaScriptが止まっちゃう

try {
  return 1;
} catch (e) {
  console.log(e);
  // 文字列を返すか？
  return 0;
}
```
