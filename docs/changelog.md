# Changelog

## 2026-05-01

- 新增好友系统和用户资料接口：用户搜索、好友申请、申请列表、同意/拒绝、好友列表、资料读取/修改、点击好友创建会话。
- 扩展 `users` 表字段：`nickname`、`signature`、`email`。
- 新增聊天记录接口：`GET /api/v1/conversations`、`GET /api/v1/messages`、`POST /api/v1/messages`。
- 新增 `MessageRouter`、`MessageService`、`MessageModel`，支持 Bearer token 鉴权、会话列表、分页历史消息和 REST 发送消息。
- 完成数据库初始化脚本，包含 `users`、`friendships`、`messages`、`sessions` 四张表，统一 InnoDB、utf8mb4、软删除和外键索引。
- 新增 Qt/C++ 后端分层：`Config`、`Database`、`JwtHelper`、`AuthRouter`、`AuthService`、`UserModel`。
- 新增 `POST /api/v1/auth/register` 注册接口，成功返回 `code: 0` 和 `data.userId`，用户名重复返回 `code: 1004`。
- 新增 `POST /api/v1/auth/login` 登录接口，成功返回 7 天有效 JWT，密码错误返回 `code: 1001`。
- 更新 `docs/api.md` 和 `docs/db_schema.md`，作为前端和测试对接依据。
