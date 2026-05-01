# API Design

## Backend Status

- 当前后端目标栈：C++17 + Qt 6.8.3。
- 本次仅完成数据库初始化模块，暂无新增 HTTP 或 WebSocket 接口。
- 数据库结构详见 `docs/db_schema.md`。

后端接口先以 FastAPI OpenAPI 文档为准，服务启动后访问：

- Swagger UI: `http://127.0.0.1:8000/docs`
- OpenAPI JSON: `http://127.0.0.1:8000/openapi.json`

## Initial Endpoints

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/health` | 服务健康检查 |

## Planned Domains

- Auth: 注册、登录、刷新令牌。
- Users: 当前用户资料、搜索用户。
- Contacts: 好友申请、好友列表、分组。
- Conversations: 单聊、群聊、会话列表。
- Messages: 历史消息、发送消息、已读回执。
- Realtime: WebSocket 在线状态、消息推送。
