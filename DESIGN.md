# dromozoa-web

## websocket

認証はクエリで行う
AWSはsignでいける？

- API GatewayでWebSocketを構築
- $request.body.action, $connect, $disconnectを作る
- Lambdaオーソライザを使えば簡単だけど
- $connectにIAM認証をつける
- WebSocketでもopenapiつかえる……よね？

コンセプト

- Lambdaを使わないで、いけるところまでいく。
- access keyは安全な方法で配布できるとする。

https://stackoverflow.com/questions/55594587/setup-a-basic-websocket-mock-in-aws-apigatewayt

送信用のAPIでCORSを有効にする

```
[
    {
        "AllowedHeaders": [
            "*"
        ],
        "AllowedMethods": [
            "GET",
            "PUT",
            "POST",
            "DELETE",
            "HEAD"
        ],
        "AllowedOrigins": [
            "http://localhost",
            "https://*.dromozoa.com/"
        ],
        "ExposeHeaders": []
    }
]
AllowedHeaders=*,AllowedMethods=GET,PUT,POST,DELETE,HEAD,AllowedOrigins=http://localhost,https://*.dromozoa.com
```

CORSはwebsocketに設定できない。

```
aws apigatewayv2 update-api --api-id ********** --cors-configuration 'AllowHeaders=*,AllowMethods=GET,PUT,POST,DELETE,HEAD,AllowOrigins=http://localhost,https://*.dromozoa.com'
```

```
aws apigatewayv2 update-integration \
    --api-id ce57u4bdl6 \
    --integration-id okrpt2q \
    --request-parameters 'integration.request.body.name'='route.request.querystring.name'
```




- リクエストとレスポンスのマッピングをよく確認する

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

- メインスレッドでエラーキューをチェックして、問題があったら、死ぬことにする
- エラーキューの内容をまず全部出力する
- そのなかで、メインのエラーでluaのエラーで終了する
- エラーキューの内容を
- tableで返す
- 結合した文字列で返す
- `error_queue:empty()`
- `#error_queue > 0`
- `tostring(error_queue)`

```
while true do
  push_error_queue()
  get_error_queue()

  assert(D.check_error_queue())

  repeat
    local e = D.get_error()
  until not e

  local e
  while true do
    local e = D.get_error()
    if not e then
      break
    end
  end


  local error_queue = D.clear_error_queue()


  local error_queue = D.pop_error_queue()
  if error_queue then
    error(tostring(error_queue))
  end

  if D.error_queue then
    D.get_error_queue()
  end

  if D.error_queue then
    error(tostring(D.error_queue))
  end
  coroutine.yield()
end
```



