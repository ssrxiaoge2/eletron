# 问题记录

## 2026-05-05 全功能回归测试 v1

## [BUG-20260505-001] 用户模块 - 注册接口未拒绝短密码
- 严重程度：P1功能错误
- 所属端：backend
- 关联用例：AuthTest::testRegisterShortPassword
- 请求：
  POST /api/v1/auth/register
  Headers: 无
  Body: { "username": "test_a", "password": "12345" }
- 预期：password 少于 6 位时 code != 0，注册失败
- 实际：code == 0，接口注册成功并返回 data.userId
- 复现步骤：
  1. 使用 tests/config/config_test.ini 启动后端，连接 im_app_test。
  2. 调用 TestDatabase::initDb() 初始化测试库。
  3. 请求 POST /api/v1/auth/register，Body 为 { "username": "test_a", "password": "12345" }。
  4. 观察响应 code == 0，短密码账号被创建。
- 备注：可能原因是注册路由或 AuthService 只校验 password 非空，未实现 docs/api.md 和测试清单要求的最小长度校验。
