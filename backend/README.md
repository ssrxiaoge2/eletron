# Backend

后端服务使用 C++17 + Qt 6.8.3 构建，负责 HTTP API、用户认证、好友关系、聊天消息和用户资料读写。

## 技术栈

- Qt Core / Network / HttpServer / WebSockets / Sql
- MySQL 8.0.46
- QMYSQL 驱动
- C++17

## 目录结构

```text
backend/
├── CMakeLists.txt
├── main.cpp
├── api/          # HTTP 服务入口
├── config/       # 配置文件示例，本地 config.ini 不提交
├── core/         # 配置、数据库、JWT 等基础模块
├── models/       # 数据库访问模型
├── routers/      # HTTP 路由
├── services/     # 业务逻辑
├── realtime/     # WebSocket 服务
└── sql/          # 后端 SQL 初始化脚本副本
```

## 本地配置

后端默认读取：

```text
backend/config/config.ini
```

本地配置文件已加入 `.gitignore`，不要提交真实密码。可参考 `backend/config/config.example.ini` 创建：

```ini
[mysql]
host=127.0.0.1
port=3306
database=im_app
username=root
password=123456

[jwt]
secret=local-development-secret-change-me
```

## 初始化数据库

在仓库根目录执行：

```powershell
mysql.exe --default-character-set=utf8mb4 -uroot -p123456 --execute="SOURCE D:/study/qtpro/admin/sql/init.sql"
```

初始化脚本会创建 `im_app` 数据库和基础测试用户：

- `testuser / 123456`
- `user1 / 123456`
- `user2 / 123456`

## 启动后端

如果已经存在 `backend/build-msvc/im_backend.exe`，可直接在仓库根目录启动：

```powershell
backend\build-msvc\im_backend.exe --port 8000 --config backend/config/config.ini
```

启动成功后服务地址为：

```text
http://127.0.0.1:8000
```

健康检查：

```powershell
Invoke-RestMethod -Uri http://127.0.0.1:8000/health -Method GET
```

## 重新编译

当前本机可用 MSVC + Qt 6.8.3 编译。仓库根目录执行：

```powershell
cmd.exe /v:on /c "call D:\study\VS\product\VC\Auxiliary\Build\vcvars64.bat && cl.exe /nologo /std:c++17 /permissive- /utf-8 /EHsc /MD /Zc:__cplusplus /DUNICODE /D_UNICODE /DQT_CORE_LIB /DQT_NETWORK_LIB /DQT_SQL_LIB /DQT_HTTPSERVER_LIB /DQT_WEBSOCKETS_LIB /I D:\study\qt\6.8.3\msvc2022_64\include /I D:\study\qt\6.8.3\msvc2022_64\include\QtCore /I D:\study\qt\6.8.3\msvc2022_64\include\QtNetwork /I D:\study\qt\6.8.3\msvc2022_64\include\QtSql /I D:\study\qt\6.8.3\msvc2022_64\include\QtHttpServer /I D:\study\qt\6.8.3\msvc2022_64\include\QtWebSockets /Fo:backend\build-msvc\manual\ /Fe:backend\build-msvc\im_backend.exe backend\main.cpp backend\api\HttpServer.cpp backend\core\Config.cpp backend\core\Database.cpp backend\core\JwtHelper.cpp backend\models\UserModel.cpp backend\models\MessageModel.cpp backend\models\FriendshipModel.cpp backend\routers\AuthRouter.cpp backend\routers\MessageRouter.cpp backend\routers\FriendRouter.cpp backend\routers\UserRouter.cpp backend\services\AuthService.cpp backend\services\MessageService.cpp backend\services\FriendService.cpp backend\services\UserService.cpp backend\realtime\WebSocketGateway.cpp /link /LIBPATH:D:\study\qt\6.8.3\msvc2022_64\lib Qt6Core.lib Qt6Network.lib Qt6Sql.lib Qt6HttpServer.lib Qt6WebSockets.lib"
```

运行目录需要包含：

```text
backend/build-msvc/im_backend.exe
backend/build-msvc/sqldrivers/qsqlmysql.dll
backend/build-msvc/libmysql.dll
```

## 登录和用户资料

登录获取 token：

```powershell
$login = Invoke-RestMethod `
  -Uri http://127.0.0.1:8000/api/v1/auth/login `
  -Method Post `
  -ContentType "application/json" `
  -Body (@{ username = "user1"; password = "123456" } | ConvertTo-Json)

$headers = @{ Authorization = "Bearer $($login.data.token)" }
```

获取当前用户资料：

```powershell
Invoke-RestMethod `
  -Uri http://127.0.0.1:8000/api/v1/user/profile `
  -Headers $headers
```

资料接口返回字段包含：

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "userId": 3,
    "username": "user1",
    "nickname": "测试用户1",
    "avatar": "",
    "status": 1,
    "gender": "unknown",
    "birthday": "2000-01-01",
    "signature": "今天也要好好聊天",
    "email": "user1@example.com",
    "createdAt": "2026-05-01"
  }
}
```

修改当前用户资料：

```powershell
Invoke-RestMethod `
  -Uri http://127.0.0.1:8000/api/v1/user/profile `
  -Method Put `
  -Headers $headers `
  -ContentType "application/json" `
  -Body (@{
    nickname = "新昵称"
    avatar = ""
    gender = "unknown"
    birthday = "2000-01-01"
    signature = "新的个性签名"
    email = "user1@example.com"
  } | ConvertTo-Json)
```

前端展示个人资料时可直接使用 `nickname`、`avatar`、`status`、`createdAt`、`gender`、`birthday`、`email`、`signature` 字段。

## 上传头像

前端当前可以继续通过 `PUT /api/v1/user/profile` 把 base64 写入 `avatar` 字段。若要改为文件 URL 模式，使用：

```powershell
Invoke-RestMethod `
  -Uri http://127.0.0.1:8000/api/v1/user/avatar `
  -Method Post `
  -Headers $headers `
  -ContentType "application/json" `
  -Body (@{
    avatar = "data:image/png;base64,iVBORw0KGgo..."
  } | ConvertTo-Json)
```

成功响应：

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

后端会把头像文件保存到 `backend/uploads/avatars/`，并把返回的 URL 写入 `users.avatar`。

## 会话唯一性

`POST /api/v1/conversations` 会写入 `conversations` 表。数据库使用 `(user_id, target_user_id)` 唯一约束，重复点击同一个好友不会创建重复会话，会直接复用已有 `conversationId`。

## 标记消息已读

前端打开会话或清除红点时调用：

```powershell
Invoke-RestMethod `
  -Uri http://127.0.0.1:8000/api/v1/messages/read `
  -Method Post `
  -Headers $headers `
  -ContentType "application/json" `
  -Body (@{ targetUserId = 4 } | ConvertTo-Json)
```

也可以调用等价接口：

```text
POST /api/v1/conversations/read
```

成功响应：

```json
{
  "code": 0,
  "message": "ok",
  "data": {
    "targetUserId": 4,
    "readCount": 3
  }
}
```

后端会把 `targetUserId` 发给当前用户的未读消息更新为 `is_read = 1`，因此软件重启后未读红点也会保持消失。
