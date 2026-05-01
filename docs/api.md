# API Design

后端服务使用 C++17 + Qt 6.8.3，HTTP 路由由 `QHttpServer` 提供，默认监听 `http://127.0.0.1:8000`。数据库访问使用 `QtSql` + `QMYSQL`。

## Endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/health` | 服务健康检查 |
| POST | `/api/v1/auth/register` | 用户注册 |
| POST | `/api/v1/auth/login` | 用户登录 |
| GET | `/api/v1/conversations` | 获取当前用户会话列表 |
| POST | `/api/v1/conversations` | 点击好友发起聊天 |
| GET | `/api/v1/messages` | 分页获取历史消息 |
| POST | `/api/v1/messages` | 发送消息 |
| GET | `/api/v1/users/search` | 搜索用户 |
| GET | `/api/v1/friends` | 获取好友列表 |
| POST | `/api/v1/friends/apply` | 发送好友申请 |
| GET | `/api/v1/friends/requests` | 获取收到的好友申请 |
| POST | `/api/v1/friends/handle` | 处理好友申请 |
| GET | `/api/v1/user/profile` | 获取当前用户资料 |
| PUT | `/api/v1/user/profile` | 修改当前用户资料 |

## Response Envelope

业务接口统一返回：

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

`code = 0` 表示成功；非 0 表示业务或服务错误。

## GET `/health`

成功响应：HTTP `200`

```json
{
  "status": "ok",
  "service": "im-backend",
  "timestamp": "2026-05-01T03:30:00Z"
}
```

## POST `/api/v1/auth/register`

请求体：

```json
{
  "username": "newuser",
  "password": "123456"
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "userId": 1001
  }
}
```

用户名重复：HTTP `409`

```json
{
  "code": 1004,
  "message": "username already exists"
}
```

## POST `/api/v1/auth/login`

请求体：

```json
{
  "username": "testuser",
  "password": "123456"
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "token": "jwt_string",
    "userId": 1001,
    "username": "testuser"
  }
}
```

账号或密码错误：HTTP `401`

```json
{
  "code": 1001,
  "message": "invalid username or password"
}
```

## Common Errors

请求体格式错误、缺少用户名或密码、字段超长：HTTP `400`

```json
{
  "code": 1000,
  "message": "username and password are required"
}
```

数据库不可用：HTTP `503`

```json
{
  "code": 2001,
  "message": "database_unavailable"
}
```

数据库查询或写入失败：HTTP `503`

```json
{
  "code": 2002,
  "message": "database_error"
}
```

JWT 签发不可用：HTTP `503`

```json
{
  "code": 2003,
  "message": "jwt_unavailable"
}
```

## GET `/api/v1/conversations`

Header:

```http
Authorization: Bearer {token}
```

功能：返回当前登录用户的全部会话，按最后一条消息时间倒序排列。

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": [
    {
      "conversationId": 2,
      "targetUserId": 2,
      "targetUsername": "user2",
      "targetAvatar": "",
      "lastMessage": "今天晚上一起聊",
      "lastMessageTime": "2024-01-01 15:27:00",
      "unreadCount": 0,
      "isOnline": true
    }
  ]
}
```

## GET `/api/v1/messages`

Header:

```http
Authorization: Bearer {token}
```

Query:

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| targetUserId | 是 | 对方用户 ID |
| page | 否 | 页码，默认 `1` |
| pageSize | 否 | 每页数量，默认 `20`，最大 `100` |

示例：

```http
GET /api/v1/messages?targetUserId=2&page=1&pageSize=20
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "total": 50,
    "page": 1,
    "pageSize": 20,
    "list": [
      {
        "messageId": 1,
        "senderId": 2,
        "receiverId": 1,
        "content": "你好，MainWindow 终于开始像一个聊天窗口了。",
        "type": 0,
        "createdAt": "2024-01-01 15:27:00",
        "isSelf": false
      }
    ]
  }
}
```

`isSelf` 由后端根据 token 对应的当前用户 ID 判断，前端可直接用于消息气泡左右对齐。

## POST `/api/v1/messages`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "receiverId": 2,
  "content": "消息内容",
  "type": 0
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "messageId": 3,
    "createdAt": "2024-01-01 15:30:00"
  }
}
```

未登录或 token 失效：HTTP `401`

```json
{
  "code": 1002,
  "message": "invalid or expired token"
}
```

## GET `/api/v1/users/search`

Header:

```http
Authorization: Bearer {token}
```

Query:

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| keyword | 是 | 按用户名或昵称模糊搜索，最多返回 20 条 |

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": [
    {
      "userId": 1002,
      "username": "dongfang",
      "nickname": "东方-Askeai",
      "avatar": "",
      "signature": "今天也要好好聊天",
      "friendStatus": 0
    }
  ]
}
```

`friendStatus`：`0` 非好友，`1` 已是好友，`2` 已发送申请，`3` 对方已申请我。

## POST `/api/v1/friends/apply`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "targetUserId": 1002
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

业务错误：

| HTTP | code | message |
| --- | --- | --- |
| 409 | 2002 | already friends |
| 409 | 2003 | friend request already sent |
| 400 | 2004 | cannot add yourself |

## GET `/api/v1/friends/requests`

Header:

```http
Authorization: Bearer {token}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": [
    {
      "requestId": 1,
      "fromUserId": 1001,
      "fromUsername": "user1",
      "fromNickname": "测试用户",
      "fromAvatar": "",
      "createdAt": "2024-01-01 15:00:00"
    }
  ]
}
```

## POST `/api/v1/friends/handle`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "requestId": 1,
  "action": 1
}
```

`action`：`1` 同意，`2` 拒绝。

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {}
}
```

## GET `/api/v1/friends`

Header:

```http
Authorization: Bearer {token}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": [
    {
      "userId": 1002,
      "username": "dongfang",
      "nickname": "东方-Askeai",
      "avatar": "",
      "signature": "今天也要好好聊天",
      "isOnline": true
    }
  ]
}
```

## GET `/api/v1/user/profile`

Header:

```http
Authorization: Bearer {token}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "userId": 1001,
    "username": "user1",
    "nickname": "测试用户",
    "avatar": "",
    "status": 1,
    "gender": "unknown",
    "birthday": "2000-01-01",
    "signature": "今天也要好好聊天",
    "email": "",
    "createdAt": "2024-01-01"
  }
}
```

## PUT `/api/v1/user/profile`

Header:

```http
Authorization: Bearer {token}
```

请求体只需要传要修改的字段：

```json
{
  "nickname": "新昵称",
  "signature": "新个性签名",
  "avatar": "",
  "gender": "unknown",
  "birthday": "2000-01-01",
  "email": ""
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "nickname": "新昵称",
    "signature": "新个性签名",
    "avatar": "",
    "gender": "unknown",
    "birthday": "2000-01-01",
    "email": ""
  }
}
```

## POST `/api/v1/conversations`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "targetUserId": 1002
}
```

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "conversationId": 1002,
    "targetUserId": 1002,
    "targetNickname": "东方-Askeai",
    "targetAvatar": "",
    "isOnline": true
  }
}
```
