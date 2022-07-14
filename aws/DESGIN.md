# dromozoa-web

## やること

- エラーのかえしかた
- `get_connection` の安全性
  - 誰かがさしこんできたのをどう判定するか
  - 乱数で一時的な秘密をさしこむ
    - DNSのトランザクションIDみたいな
    - どれくらいにする？
  - nonce

## メモ

名前づけの規則
- DNS名に使われる場合だけ、`-` を使う
- それ以外のときは `_` を使う

API
- `dromozoa_web`
- `dromozoa_web_socket`

Stage
- d / development
- p / production

DynamoDB
- `dromozoa_web_connections`
  - `id`
  - `name`
  - `public_key`

- `connections/{connection_id}`
- `socket_connections`

- `connections_url`
- `socket_url`

REST API
- `/{stage}/@connections/{connection_id}`
  - GET
  - POST
  - DELETE
  - OPTIONS
  - `x-amz-content-sha256`
  - `x-amz-date`
  - `x-dromozoa-web-socket-connections-authorization`

WebSocket API
- action=`get_connection`
  - id

