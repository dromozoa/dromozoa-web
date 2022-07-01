# dromozoa-web

## エラーの伝搬

- JavaScriptの例外
- Luaのエラー
- C++の例外

- メインループのなかで発生したエラー
  - メインコルーチンのなかで発生したエラー
- メインループのそとで発生したエラー

- エラーからのリカバリは行わない
- エラー情報の出力は同期的にconsole.logに出力する

- `JS_ASM`呼び出しによるJavaScript例外はC++例外に変換する
- Luaから呼び出されるC++関数のエラーはLuaエラーに変換する
- pcall結果のエラーはコンテキスト依存する
  - pcallがJavaScriptから呼びだされるとき、呼び出し側でJavaScriptに変換する

```
-- D.asyncはcoroutine.createして、一度、coroutine.resumeをする
-- D.awaitは、coroutine.resumeされるまで待つ
-- つまり、coroutine.yieldして待つ
-- coroutine.yieldの結果がエラーだったら、コルーチンを落とす

local thread_wrapper = D.async(function (thread)
  local x = D.await(js promise)
  local y = D.await(fetch (url)
    :then_(function (v) end)
    :then_(function (v) end)
    :then_(function (v) coroutine.resume(thread, resolved, v) end)
    :catch(function (e) coroutine.resume(thread, rejected, e) end)
  return xxx
end)

```

- promise.then_は初期にインストールしてしまうか？
- luacの結果をインクルードする？

