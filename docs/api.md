# API Design

后端服务使用 C++17 + Qt 6.8.3，HTTP 路由由 `QHttpServer` 提供，默认监听 `http://127.0.0.1:8000`。数据库访问使用 `QtSql` + `QMYSQL`。

## Endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/health` | 服务健康检查 |
| POST | `/api/v1/auth/register` | 用户注册 |
| POST | `/api/v1/auth/login` | 用户登录 |
| POST | `/api/v1/auth/logout` | 用户登出 |
| POST | `/api/v1/auth/offline` | 用户仅下线，保留快捷登录凭证 |
| GET | `/api/v1/conversations` | 获取当前用户会话列表 |
| POST | `/api/v1/conversations` | 点击好友发起聊天 |
| GET | `/api/v1/messages` | 分页获取历史消息 |
| POST | `/api/v1/messages` | 发送消息 |
| POST | `/api/v1/messages/read` | 标记与指定用户的消息已读 |
| POST | `/api/v1/conversations/read` | 标记指定会话已读 |
| GET | `/api/v1/users/search` | 搜索用户 |
| GET | `/api/v1/friends` | 获取好友列表 |
| POST | `/api/v1/friends/apply` | 发送好友申请 |
| GET | `/api/v1/friends/requests` | 获取收到的好友申请 |
| POST | `/api/v1/friends/handle` | 处理好友申请 |
| GET | `/api/v1/user/profile` | 获取当前用户资料 |
| PUT | `/api/v1/user/profile` | 修改当前用户资料 |
| POST | `/api/v1/user/avatar` | 上传当前用户头像 |
| POST | `/api/v1/files/upload` | 上传文件或图片并生成消息 |
| GET | `/api/v1/files/download/{fileId}` | 下载文件 |
| GET | `/api/v1/files/thumbnail/{fileId}` | 获取图片缩略图 |
| GET | `/api/v1/files/info/{fileId}` | 获取文件信息 |

| POST | `/api/v1/groups` | 创建群聊 |
| GET | `/api/v1/groups` | 获取我加入的群列表 |
| GET | `/api/v1/groups/{groupId}` | 获取群信息 |
| DELETE | `/api/v1/groups/{groupId}` | 解散群聊 |
| PUT | `/api/v1/groups/{groupId}/name` | 修改群名称 |
| PUT | `/api/v1/groups/{groupId}/announcement` | 发布群公告 |
| GET | `/api/v1/groups/{groupId}/members` | 获取群成员列表 |
| PUT | `/api/v1/groups/{groupId}/members/{userId}/role` | 设置或取消管理员 |
| DELETE | `/api/v1/groups/{groupId}/members/{userId}` | 踢出成员 |
| PUT | `/api/v1/groups/{groupId}/members/{userId}/mute` | 禁言成员 |
| DELETE | `/api/v1/groups/{groupId}/members/self` | 退出群聊 |
| POST | `/api/v1/groups/{groupId}/messages` | 发送群消息 |
| GET | `/api/v1/groups/{groupId}/messages` | 获取群历史消息 |

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
登录成功后，后端会将当前用户 `users.status` 更新为 `1`，后续好友列表和会话列表会通过 `isOnline: true` 体现在线状态。

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

当前用户已有有效 session：HTTP `409`。后端不会创建新的 session。

```json
{
  "code": 1005,
  "message": "current user already logged in"
}
```

## POST `/api/v1/auth/logout`

Header:

```http
Authorization: Bearer {token}
```

行为：同一事务中将当前 session 失效，并把当前用户 `users.status` 更新为 `0`。接口支持重复调用；token 已失效或缺失时也返回成功，方便前端关闭窗口时静默调用。

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok"
}
```

## POST `/api/v1/auth/offline`

Header:

```http
Authorization: Bearer {token}
```

行为：仅将当前用户 `users.status` 更新为 `0`，不会删除或失效当前 `session`，用于前端普通关闭主窗口时让用户下线，同时保留本机快捷登录凭证。接口支持重复调用；token 缺失、已失效或找不到对应 session 时也返回成功。

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok"
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
`isOnline` 固定返回 JSON 布尔值：`true` 表示在线，`false` 表示离线或隐身。

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
      "lastMessageType": 0,
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

消息 `type` 字段含义：

- `0`：文字消息，`content` 为纯文本。
- `1`：图片消息，`content` 为文件 JSON 字符串，包含 `fileId`、`fileName`、`fileSize`、`url`、`thumbnailUrl`。
- `2`：文件消息，`content` 为文件 JSON 字符串，包含 `fileId`、`fileName`、`fileSize`、`url`、`thumbnailUrl`。

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

## POST `/api/v1/messages/read`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "targetUserId": 2
}
```

功能：将 `targetUserId` 发给当前登录用户的未读消息持久化标记为已读。前端打开会话或清除红点时调用该接口，重启软件后红点不会恢复。

成功响应：HTTP `200`

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "targetUserId": 2,
    "readCount": 3
  }
}
```

`POST /api/v1/conversations/read` 是同等能力的别名，请求体和响应完全一致。

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
`isOnline` 固定返回 JSON 布尔值：`true` 表示在线，`false` 表示离线或隐身。

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

## POST `/api/v1/user/avatar`

Header:

```http
Authorization: Bearer {token}
```

请求体支持前端传入 base64 图片，字段名可用 `avatar` 或 `imageBase64`：

```json
{
  "avatar": "data:image/png;base64,iVBORw0KGgo..."
}
```

成功响应：HTTP `200`。后端会保存头像文件、更新 `users.avatar`，并返回可用于展示的 URL。

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "avatar": "/uploads/avatars/user_3_xxx.png",
    "avatarUrl": "/uploads/avatars/user_3_xxx.png"
  }
}
```

## POST `/api/v1/files/upload`

Header:

```http
Authorization: Bearer {token}
Content-Type: multipart/form-data
```

表单字段：

| 字段 | 说明 |
| --- | --- |
| file | 文件二进制内容 |
| type | `1` 图片，`2` 普通文件 |
| receiverId | 接收方用户 ID |
| groupId | 群组 ID。传值时表示群文件或群图片上传，此时 `receiverId` 可不传或传 `0` |

成功响应：HTTP `200`。上传成功后，后端会自动写入一条 `messages` 记录：图片消息 `type=1`，普通文件消息 `type=2`，`content` 为文件 JSON 字符串。

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "fileId": 1001,
    "fileName": "screenshot.png",
    "fileSize": 204800,
    "fileType": 1,
    "mimeType": "image/png",
    "uploaderUserId": 1001,
    "url": "/api/v1/files/download/1001",
    "thumbnailUrl": "/api/v1/files/thumbnail/1001",
    "createdAt": "2024-01-01 15:30:00"
  }
}
```

文件超过大小限制：HTTP `413`

```json
{
  "code": 4001,
  "message": "file exceeds size limit"
}
```

不支持的文件类型：HTTP `400`

```json
{
  "code": 4003,
  "message": "unsupported file type"
}
```

## GET `/api/v1/files/download/{fileId}`

Header:

```http
Authorization: Bearer {token}
```

功能：验证 token，并确认当前用户是文件发送方或接收方之一。成功时返回文件二进制流。

成功响应 Header：

```http
Content-Type: application/octet-stream
Content-Disposition: attachment; filename="原始文件名"
Content-Length: 文件大小
```

无权访问：HTTP `403`

```json
{
  "code": 1003,
  "message": "无权访问"
}
```

文件不存在：HTTP `404`

```json
{
  "code": 4002,
  "message": "file not found"
}
```

## GET `/api/v1/files/thumbnail/{fileId}`

Header:

```http
Authorization: Bearer {token}
```

功能：返回图片缩略图二进制流，缩略图最大 `200x200px`。普通文件或文件不存在返回 `4002`。

成功响应 Header：

```http
Content-Type: image/png
Content-Length: 缩略图字节数
```

## GET `/api/v1/files/info/{fileId}`

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
    "fileId": 1001,
    "fileName": "document.pdf",
    "fileSize": 1048576,
    "fileType": 2,
    "mimeType": "application/pdf",
    "uploaderUserId": 1001,
    "url": "/api/v1/files/download/1001",
    "createdAt": "2024-01-01 15:30:00"
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
`isOnline` 固定返回 JSON 布尔值：`true` 表示在线，`false` 表示离线或隐身。

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "conversationId": 1,
    "targetUserId": 1002,
    "targetNickname": "东方-Askeai",
    "targetAvatar": "",
    "isOnline": true
  }
}
```

后端使用 `conversations` 表维护会话记录，并通过 `(user_id, target_user_id)` 唯一约束避免重复会话。重复调用会返回同一条会话记录。
## POST `/api/v1/groups`

Header:

```http
Authorization: Bearer {token}
```

请求体：

```json
{
  "name": "我的群",
  "memberIds": [1002, 1003]
}
```

成功响应：

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "groupId": 1,
    "name": "我的群",
    "memberCount": 3,
    "createdAt": "2024-01-01 15:00:00"
  }
}
```

## GET `/api/v1/groups`

```json
{
  "code": 0,
  "message": "ok",
  "data": [
    {
      "groupId": 1,
      "name": "我的群",
      "avatar": "",
      "lastMessage": "你好",
      "lastMessageType": 0,
      "lastMessageTime": "2024-01-01 15:00:00",
      "unreadCount": 0,
      "myRole": 2,
      "memberCount": 3
    }
  ]
}
```

## GET `/api/v1/groups/{groupId}`

```json
{
  "code": 0,
  "data": {
    "groupId": 1,
    "name": "我的群",
    "ownerId": 1001,
    "announcement": "群公告内容",
    "memberCount": 3,
    "maxMember": 100,
    "avatar": "",
    "createdAt": "2024-01-01"
  }
}
```

## DELETE `/api/v1/groups/{groupId}`

仅群主可调用。成功返回：

```json
{
  "code": 0,
  "message": "ok"
}
```

## PUT `/api/v1/groups/{groupId}/name`

```json
{
  "name": "新群名"
}
```

## PUT `/api/v1/groups/{groupId}/announcement`

```json
{
  "announcement": "公告内容"
}
```

## GET `/api/v1/groups/{groupId}/members`

```json
{
  "code": 0,
  "data": [
    {
      "userId": 1001,
      "username": "user1",
      "nickname": "测试用户",
      "avatar": "",
      "role": 2,
      "isMuted": false,
      "muteUntil": null,
      "joinedAt": "2024-01-01"
    }
  ]
}
```

## PUT `/api/v1/groups/{groupId}/members/{userId}/role`

```json
{
  "role": 1
}
```

## DELETE `/api/v1/groups/{groupId}/members/{userId}`

群主可以踢除自己外的任何成员；管理员只能踢普通群员。

## PUT `/api/v1/groups/{groupId}/members/{userId}/mute`

```json
{
  "duration": 3600
}
```

`duration = 0` 表示解除禁言。

## DELETE `/api/v1/groups/{groupId}/members/self`

群主不能退出群聊，应调用解散接口。错误码：`5004`。

## POST `/api/v1/groups/{groupId}/messages`

```json
{
  "content": "消息内容",
  "type": 0
}
```

禁言中返回：

```json
{
  "code": 5005,
  "message": "您已被禁言，禁言到期时间：2024-01-01 16:00:00",
  "data": {
    "muteUntil": "2024-01-01 16:00:00"
  }
}
```

## GET `/api/v1/groups/{groupId}/messages`

```json
{
  "code": 0,
  "data": {
    "total": 100,
    "page": 1,
    "pageSize": 20,
    "list": [
      {
        "messageId": 1,
        "senderId": 1002,
        "senderNickname": "东方-Askeai",
        "senderAvatar": "",
        "senderRole": 1,
        "content": "你好",
        "type": 0,
        "createdAt": "2024-01-01 15:00:00",
        "isSelf": false
      }
    ]
  }
}
```

## 群文件上传

复用 `POST /api/v1/files/upload`。群聊场景新增表单字段 `groupId`，此时 `receiverId` 可不传或传 `0`。上传成功后会写入 `group_messages`，`content` 与私聊文件消息保持相同的文件 JSON 字符串结构。

## Group Errors

```json
{ "code": 5001, "message": "target user is not your friend" }
{ "code": 5002, "message": "group member limit exceeded" }
{ "code": 5003, "message": "permission denied" }
{ "code": 5004, "message": "group owner cannot leave group" }
```
