# Issues

## 2026-05-01

- 已处理：登录接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/login`。
- 已处理：注册接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/register`。
  前端已接入真实注册请求。
- 待优化：后端尚未提供 session/token 快捷登录接口。前端当前为了支持已登录账号
  的默认登录视图，临时使用本地缓存的上次密码完成快捷登录；后续应改为 token
  或设备会话机制。
