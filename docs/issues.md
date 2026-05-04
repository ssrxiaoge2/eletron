# Issues

## [FE-BE-AUTH-002] Quick login conflicts with logout invalidating token

- Severity: P1 functional bug
- Owner: backend
- Frontend symptom: after the user logs in, closes the main window, and restarts the app, quick login reports that authentication is expired.
- Current frontend handling: on the same machine, the frontend now holds a per-user lock after login, so another frontend process trying to log in with the same username shows `当前用户已登录` before entering the main window. If `POST /api/v1/auth/login` returns `code: 1005` or an equivalent already-online message, the login window also shows `当前用户已登录`.
- Current API conflict: `POST /api/v1/auth/logout` is required on window close to mark the user offline, but `docs/api.md` says logout invalidates the current session. A cached session token cannot both be invalidated on close and remain usable for quick login on next startup.
- Backend request:
  - Provide a documented way to mark the user offline while keeping the cached quick-login credential reusable, or add a refresh/quick-login credential that survives logout and can issue a new session token.
  - Keep `POST /api/v1/auth/logout` for explicit logout if session invalidation is still needed.
- Frontend follow-up: after the API contract is updated, the frontend can call the offline-preserving endpoint on normal window close and keep using the cached quick-login credential on next startup.

## [FE-BE-AUTH-001] Login should reject already-online user

- Severity: P1 functional bug
- Owner: backend
- Frontend symptom: the same account can log in again while already online, which causes session and UI state bugs.
- Current frontend handling: if `POST /api/v1/auth/login` returns HTTP `409`, code `1005`/`1006`, or a message containing "already logged/login/online", the login window shows `当前用户已登录` and blocks entering the main window.
- Backend request:
  - In `POST /api/v1/auth/login`, before creating a new session, check whether the target user is already online.
  - If already online, do not create another session and return a documented conflict response, preferably HTTP `409` with `code: 1005` and `message: "current user already logged in"`.
  - Write the final error contract into `docs/api.md`.

## [FE-BE-ONLINE-001] Window close needs logout/offline API

- Severity: P1 functional bug
- Owner: backend
- Frontend symptom: after the desktop main window is closed, other clients still see the user as online.
- Current frontend limitation: `docs/api.md` has `POST /api/v1/auth/login`, but no logout/offline endpoint. The frontend cannot persistently update `users.status` without a documented backend API.
- Backend request:
  - Add `POST /api/v1/auth/logout`.
  - Header: `Authorization: Bearer {token}`.
  - Behavior: invalidate/delete the current session and update the current user's `users.status = 0` in the same transaction.
  - Success response: `{ "code": 0, "message": "ok" }`.
  - The endpoint should be idempotent enough that repeated close/logout calls do not cause an error visible to the user.
- Frontend follow-up: after this endpoint is written into `docs/api.md`, `MainWindow::closeEvent` should call `POST /api/v1/auth/logout`, stop polling timers, clear the local token, and then close.

## 2026-05-01 全模块接口测试

## [BUG-001] GET /api/v1/user/profile - 用户资料缺少 email 字段
- 严重程度：P3 格式问题
- 所属模块：backend
- 测试用例：UserTest::testGetProfile
- 请求信息：
  GET /api/v1/user/profile
  Header: Authorization: Bearer {valid_token}
- 预期结果：code == 0，data 中包含 userId / username / nickname / signature / email
- 实际结果：code == 0，但 data 中缺少 email 字段
- 复现步骤：
  1. 初始化 im_app_test 数据库
  2. 注册并登录用户 testuser1，获取有效 token
  3. 使用该 token 请求 GET /api/v1/user/profile
  4. 检查响应 data 字段，未发现 email

## [BUG-002] POST /api/v1/messages - 非好友可发送消息
- 严重程度：P1 功能错误
- 所属模块：backend
- 测试用例：MessageTest::testSendMessageToStranger
- 请求信息：
  POST /api/v1/messages
  Body: { "receiverId": userBId, "content": "你好", "type": 0 }
- 预期结果：向非好友发送消息返回错误码（非 0）
- 实际结果：code == 0，返回 messageId 和 createdAt
- 复现步骤：
  1. 初始化 im_app_test 数据库
  2. 注册并登录用户 A、用户 B，但不建立好友关系
  3. 用户 A 请求 POST /api/v1/messages，receiverId 为用户 B
  4. 响应 code == 0，消息发送成功

## 2026-05-01

- 已处理：登录接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/login`。
- 已处理：注册接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/register`。
- 已处理：快捷登录接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/quick-login`。前端应缓存 `session.token`，不要缓存明文密码。
