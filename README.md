# 飞鸽即时通讯

飞鸽即时通讯是一个基于 Qt/C++ 的桌面即时通讯项目，采用前后端分离架构。前端负责登录、联系人、会话列表、单聊、群聊、文件传输等桌面交互；后端负责用户认证、好友关系、群组管理、消息存储、文件接口和实时通信通道。

项目同时引入 Codex 多智能体协作开发方式，将开发职责拆分为前端开发智能体、后端开发智能体和测试智能体，配合 Git 分支推进功能迭代、接口联调和回归验证。

## 技术栈

- 语言与框架：C++17、Qt 6
- 前端：Qt Widgets、Qt Network、Qt WebSockets
- 后端：Qt Core、Qt Network、Qt Sql、Qt HttpServer、Qt WebSockets
- 数据库：MySQL、QMYSQL 驱动、InnoDB
- 构建工具：CMake、Visual Studio / MSVC、MinGW
- 协作工具：Git、GitHub、Codex 多智能体
- 测试：Qt Test、接口测试、集成流程测试

## 项目结构

```text
im-app/
├── frontend/             # Qt 桌面客户端
│   ├── src/              # 主窗口、登录、聊天、网络客户端等核心代码
│   ├── widgets/          # 好友、群聊、资料、文件传输等 UI 组件
│   ├── resources/        # Qt 资源文件
│   ├── styles/           # QSS 样式
│   ├── CMakeLists.txt    # 前端 CMake 构建入口
│   └── admin.vcxproj     # Visual Studio Qt 工程
├── backend/              # Qt/C++ 后端服务
│   ├── api/              # HTTP 服务入口与路由注册
│   ├── routers/          # 各业务 HTTP 路由
│   ├── services/         # 业务逻辑层
│   ├── models/           # 数据访问与数据组装
│   ├── core/             # 配置、数据库、JWT、文件工具
│   ├── realtime/         # WebSocket 网关
│   ├── config/           # 本地配置示例
│   ├── storage/          # 文件、图片、缩略图存储目录
│   └── CMakeLists.txt    # 后端 CMake 构建入口
├── sql/                  # MySQL 初始化脚本
├── tests/                # Qt Test 单元测试与集成测试
├── docs/                 # API、数据库、测试报告和变更记录
└── README.md
```

## 架构设计

项目采用典型的前后端分离设计：

```text
Qt Widgets Client
    │
    │ HTTP JSON / multipart
    ▼
Qt HttpServer Backend
    │
    ├── Routers    路由层：解析请求、参数校验、响应封装
    ├── Services   服务层：认证、好友、群组、消息、文件业务规则
    ├── Models     数据层：SQL 查询、数据写入、JSON 数据组装
    └── Core       基础设施：MySQL 连接、JWT、配置、文件存储
    │
    ▼
MySQL
```

当前核心业务链路主要使用 HTTP API 完成闭环，WebSocket 网关已经预留在 `backend/realtime/`，用于后续扩展服务端主动推送、在线状态广播和实时消息通知。

## 前端模块设计

前端是 Qt Widgets 桌面客户端，核心代码位于 `frontend/src/` 和 `frontend/widgets/`。

### 核心模块

- `LoginWindow`：登录窗口，负责账号密码登录、缓存 token 快捷登录、注册入口。
- `AuthClient`：认证接口客户端，封装登录、注册、快捷登录请求。
- `ApiClient`：通用 HTTP 客户端，统一处理 token、GET/POST/PUT/DELETE、文件上传下载和回调。
- `MainWindow`：主窗口，管理导航栏、联系人、会话、群组等主要页面。
- `ChatWindow`：单聊窗口，负责消息展示、发送、刷新、文件和图片消息交互。
- `GroupChatWindow`：群聊窗口，负责群消息展示、发送、成员侧栏、公告、禁言等群聊交互。
- `ChatListWidget`：会话列表，负责单聊和群聊入口展示。
- `FriendManagerWidget` / `FriendTabWidget`：好友管理、好友分组和好友申请处理。
- `GroupTabWidget` / `GroupMemberWidget`：群组列表、群成员、群权限管理。
- `MessageBubble` / `GroupMessageBubble`：聊天气泡组件。

### 前端网络机制

前端通过 `QNetworkAccessManager` 发起异步 HTTP 请求，请求完成后通过 Qt signal/slot 或 lambda 回调更新 UI，避免阻塞主线程。

登录成功后，`ApiClient` 保存 token，后续业务请求统一携带：

```http
Authorization: Bearer <token>
```

文件传输使用 `multipart/form-data` 上传，下载通过文件接口获取二进制数据，并支持上传/下载进度回调。

## 后端模块设计

后端是 Qt/C++ 服务端程序，入口位于 `backend/main.cpp`。启动后会读取配置、连接 MySQL，并通过 `QHttpServer` 监听 HTTP 请求。

### 分层说明

- `api/HttpServer.*`：创建 HTTP 服务，注册健康检查、认证、用户、好友、群组、消息、文件等路由。
- `routers/`：路由层，只处理请求解析、基础参数校验和响应包装。
- `services/`：业务层，负责 token 鉴权、权限判断、好友关系校验、群成员校验、禁言校验等规则。
- `models/`：数据层，负责 SQL 查询、插入、更新和 JSON 结果组装。
- `core/Database.*`：封装 MySQL 连接，使用 QMYSQL 驱动并设置 `utf8mb4`。
- `core/JwtHelper.*`：生成 JWT，提供 SHA-256 和 HMAC-SHA256 工具。
- `core/FileHelper.*`：处理文件名清洗、唯一文件名、文件保存、图片缩略图生成。
- `realtime/WebSocketGateway.*`：WebSocket 服务网关，目前作为实时推送能力的基础设施预留。

### 主要接口领域

- 认证：注册、登录、退出、离线
- 用户：个人资料查询与修改
- 好友：好友申请、同意/拒绝、好友列表、好友分组
- 消息：单聊消息发送、历史消息、会话列表、已读状态
- 群组：创建群、群列表、成员管理、公告、禁言、群聊消息
- 文件：图片/文件上传、下载、缩略图、文件信息

## 核心业务流程

### 登录流程

```text
LoginWindow
  -> AuthClient::Login
  -> POST /api/v1/auth/login
  -> AuthRouter
  -> AuthService::login
  -> users 表校验账号密码
  -> sessions 表写入 token_hash 和过期时间
  -> users.status 更新为在线
  -> 前端保存 token
  -> 打开 MainWindow
```

后端不会保存 token 明文，而是保存 token 的 SHA-256 值到 `sessions.token_hash`。后续请求通过 `Authorization: Bearer <token>` 传入 token，服务端对 token 做 SHA-256 后查询 session。

### 单聊消息流程

```text
ChatWindow 点击发送
  -> ApiClient POST /api/v1/messages
  -> MessageRouter
  -> MessageService::sendMessage
  -> 校验 token、接收者、好友关系、消息内容
  -> MessageModel::insertMessage
  -> messages 表追加一条消息
  -> 返回 messageId 和 createdAt
  -> 前端更新聊天气泡
```

当前消息持久化采用 append-only 追加写，避免多客户端同时发消息时竞争同一行数据。

### 群聊消息流程

```text
GroupChatWindow 点击发送
  -> POST /api/v1/groups/{groupId}/messages
  -> GroupRouter
  -> GroupService::sendMessage
  -> 校验 token、群成员身份、禁言状态
  -> GroupMessageModel::insert
  -> group_messages 表按 group_id 写入一条消息
  -> 群成员按 groupId 拉取历史消息
```

群聊消息在数据库层只保存一份，不会给每个群成员复制一条记录。后续 WebSocket 推送接入后，可以在消息落库成功后遍历在线群成员连接进行实时推送。

### 文件传输流程

```text
前端选择文件
  -> ApiClient::uploadFile
  -> POST /api/v1/files/upload
  -> FileRouter 解析 multipart
  -> FileService::upload
  -> 校验 token、好友关系或群成员权限
  -> 保存文件到 backend/storage
  -> 图片生成缩略图
  -> files 表写入文件元数据
  -> messages 或 group_messages 写入文件类型消息
```

当前版本采用一次性 multipart 上传，图片最大 20MB，普通文件最大 100MB。暂未实现大文件分片、断点续传和秒传。

## 数据库设计

数据库初始化脚本位于 `sql/init.sql`，核心表如下：

- `users`：用户基础信息，包含账号、密码哈希、昵称、头像、在线状态等。
- `sessions`：登录会话，保存 token_hash、过期时间和失效状态。
- `friend_groups`：用户自定义好友分组。
- `friendships`：好友关系，记录申请、同意、拒绝和所属分组。
- `messages`：单聊消息，包含发送者、接收者、内容、类型、已读状态。
- `groups`：群组基础信息，包含群主、公告、人数、头像和解散状态。
- `group_members`：群成员关系，包含角色、禁言状态、退群状态。
- `group_messages`：群聊消息，按 `group_id` 存储群消息。
- `files`：文件元数据，包含上传者、接收者或群组、文件名、存储路径、缩略图路径等。

数据库使用 InnoDB，消息写入以追加为主；文件上传涉及文件元数据和消息记录两步写入时使用事务保证一致性。

## 多智能体协作设计

项目按职责拆分为三个 Codex 智能体：

- 前端开发智能体：维护 `frontend/`，负责 Qt UI、客户端状态、接口接入和交互体验。
- 后端开发智能体：维护 `backend/`、`sql/`，负责 HTTP API、业务逻辑、数据持久化和服务端能力。
- 测试智能体：维护 `tests/` 和测试报告，负责接口测试、集成流程测试和回归验证。

协作原则：

- 跨模块修改需要同步更新 `docs/` 或 README。
- 接口变更由后端智能体更新文档，前端智能体按文档接入。
- 测试智能体在功能合并后补充回归用例，降低多分支迭代风险。

## 运行方式

### 1. 初始化数据库

确保 MySQL 已启动后，在项目根目录执行：

```powershell
mysql.exe --default-character-set=utf8mb4 -uroot -p --execute="SOURCE D:/study/qtpro/admin/sql/init.sql"
```

根据本地环境创建后端配置文件：

```text
backend/config/config.ini
```

示例：

```ini
[mysql]
host=127.0.0.1
port=3306
database=im_app
username=root
password=你的数据库密码

[jwt]
secret=please-change-this-secret
```

### 2. 启动后端

如果已有构建产物，可直接运行：

```powershell
backend\build-msvc\im_backend.exe --port 8000 --config backend/config/config.ini
```

如果需要重新构建：

```powershell
cmake -S backend -B backend/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:/study/qt/6.8.3/msvc2022_64"
cmake --build backend/build --config Debug
backend\build\Debug\im_backend.exe --port 8000 --config backend/config/config.ini
```

健康检查：

```powershell
Invoke-RestMethod -Uri http://127.0.0.1:8000/health
```

### 3. 启动前端

方式一：使用 Visual Studio 打开：

```text
frontend/admin.slnx
```

选择 `x64 Debug` 或 `x64 Release` 后运行。

方式二：使用 CMake 构建：

```powershell
cmake -S frontend -B frontend/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:/study/qt/6.8.3/msvc2022_64"
cmake --build frontend/build --config Debug
frontend\build\Debug\qq_desktop_frontend.exe
```

前端默认访问：

```text
http://127.0.0.1:8000
```

### 4. 运行测试

测试工程位于 `tests/`，需要先启动后端测试服务并准备测试数据库配置。

```powershell
cmake -S tests -B tests/build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:/study/qt/6.8.3/msvc2022_64"
cmake --build tests/build --config Debug
ctest --test-dir tests/build -C Debug --output-on-failure
```

## 当前实现边界

- 当前核心消息链路以 HTTP API 和客户端刷新拉取为主，WebSocket 主要作为实时推送网关预留。
- 文件上传暂未做分片和断点续传。
- 当前没有显式使用 `QThread` 或 `std::thread`，主要依赖 Qt 事件循环和异步网络请求避免 UI 阻塞。
- 密码当前使用 SHA-256 哈希，后续生产化可升级为带盐哈希算法。

## 文档索引

- API 文档：`docs/api.md`
- 数据库说明：`docs/db_schema.md`
- 多智能体协作：`docs/agents.md`
- 测试报告：`docs/test_report.md`
- 变更记录：`docs/changelog.md`

