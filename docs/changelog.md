# Changelog

## 2026-05-01

- 新增 `POST /api/v1/auth/logout`，支持关闭窗口时失效当前 session 并将用户在线状态更新为离线，重复调用返回成功。
- 修复缓存 token 直接进入主界面时在线状态不刷新的问题；现在有效 token 访问资料、好友、会话、消息等接口时也会刷新 `users.status = 1`。
- 修复登录成功后未更新 `users.status` 的问题；现在登录会在同一事务内创建 session 并将当前用户置为在线状态 `1`。
- 新增 `POST /api/v1/messages/read` 和 `POST /api/v1/conversations/read`，支持持久化清除会话未读红点。
- 新增 `conversations` 表，使用 `(user_id, target_user_id)` 唯一约束保证 `POST /api/v1/conversations` 不产生重复会话。
- 新增 `POST /api/v1/user/avatar`，支持 base64 头像上传，保存文件后返回头像 URL 并更新 `users.avatar`。
- 修复 `GET /api/v1/user/profile` 响应缺少 `email` 字段的问题，并在资料更新接口中保留 `email` 兼容。
- 调整 `users` 表为前端资料显示字段：登录用户名、密码哈希、昵称、在线状态、头像、注册时间、软删除、性别、生日、个性签名；资料接口支持修改性别、生日和 `email` 兼容字段。
- 新增好友系统和用户资料接口：用户搜索、好友申请、申请列表、同意/拒绝、好友列表、资料读取/修改、点击好友创建会话。
- 扩展 `users` 表字段：`nickname`、`signature`、`gender`、`birthday`。
- 新增聊天记录接口：`GET /api/v1/conversations`、`GET /api/v1/messages`、`POST /api/v1/messages`。
- 新增 `MessageRouter`、`MessageService`、`MessageModel`，支持 Bearer token 鉴权、会话列表、分页历史消息和 REST 发送消息。
- 完成数据库初始化脚本，包含 `users`、`friendships`、`messages`、`sessions` 四张表，统一 InnoDB、utf8mb4、软删除和外键索引。
- 新增 Qt/C++ 后端分层：`Config`、`Database`、`JwtHelper`、`AuthRouter`、`AuthService`、`UserModel`。
- 新增 `POST /api/v1/auth/register` 注册接口，成功返回 `code: 0` 和 `data.userId`，用户名重复返回 `code: 1004`。
- 新增 `POST /api/v1/auth/login` 登录接口，成功返回 7 天有效 JWT，密码错误返回 `code: 1001`。
- 更新 `docs/api.md` 和 `docs/db_schema.md`，作为前端和测试对接依据。
