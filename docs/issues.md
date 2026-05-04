# 问题记录

## [FE-BE-AUTH-002] 快捷登录与 logout 失效 token 冲突

- 严重程度：P1 功能错误
- 负责人：backend
- 现象：用户登录后关闭主窗口，再次启动应用使用快捷登录时提示认证已过期。
- 当前前端处理：同一台机器上，前端登录成功后会持有按用户名区分的本机会话锁；另一个前端进程尝试登录同一用户名时，会在进入主界面前提示「当前用户已登录」。如果 `POST /api/v1/auth/login` 返回 `code: 1005` 或等价的已在线提示，登录窗口也会提示「当前用户已登录」。
- 当前接口冲突：关闭窗口时需要调用 `POST /api/v1/auth/logout` 让用户下线，但 `docs/api.md` 明确说明 logout 会让当前 session 失效。被失效的 session token 无法在下次启动时继续作为快捷登录凭证使用。
- 后端需求：
  - 提供一个已写入 `docs/api.md` 的“仅下线但保留快捷登录凭证”的接口，或提供一个 logout 后仍可用于换取新 session token 的刷新/快捷登录凭证。
  - 如果仍需要让 `POST /api/v1/auth/logout` 表示显式登出，可以保留它当前的 session 失效语义。
- 前端后续处理：接口契约更新后，前端会在普通关闭窗口时调用“仅下线”接口，并在下次启动时继续使用缓存的快捷登录凭证。

## [FE-BE-AUTH-001] 登录接口需要拒绝已在线用户

- 严重程度：P1 功能错误
- 负责人：backend
- 现象：同一账号已经在线时仍可再次登录，导致 session 和界面状态异常。
- 当前前端处理：同一台机器上，前端通过本机会话锁阻止同一用户名重复进入主界面；如果 `POST /api/v1/auth/login` 返回 `code: 1005` 或等价的已在线提示，登录窗口会提示「当前用户已登录」。
- 后端需求：
  - 在 `POST /api/v1/auth/login` 创建新 session 前，检查目标用户是否已经在线。
  - 如果已经在线，不创建新的 session，并返回文档化的冲突响应，建议 HTTP `409`、`code: 1005`、`message: "current user already logged in"`。
  - 将最终错误契约写入 `docs/api.md`。

## [FE-BE-ONLINE-001] 关闭窗口需要下线接口

- 严重程度：P1 功能错误
- 负责人：backend
- 现象：桌面主窗口关闭后，其他客户端仍看到该用户在线。
- 当前前端限制：没有文档化下线接口时，前端无法持久化修改 `users.status`。
- 后端需求：
  - 新增并文档化 `POST /api/v1/auth/logout`。
  - Header: `Authorization: Bearer {token}`。
  - 行为：在同一事务中让当前 session 失效，并将当前用户 `users.status` 更新为 `0`。
  - 成功响应：`{ "code": 0, "message": "ok" }`。
  - 接口应支持重复调用，不向用户暴露错误。
- 前端后续处理：接口写入 `docs/api.md` 后，`MainWindow::closeEvent` 调用 `POST /api/v1/auth/logout`，停止轮询，清空本地 token，然后关闭。

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
