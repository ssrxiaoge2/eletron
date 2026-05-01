# Issues

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
