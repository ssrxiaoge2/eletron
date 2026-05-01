# Issues

## 2026-05-01

- 已处理：登录接口已写入 `docs/api.md`，并在 Qt 后端实现 `POST /auth/login`。
  前端可继续调用 `POST http://127.0.0.1:8000/auth/login`，请求体为
  `{ "username": "...", "password": "..." }`。
  成功返回 HTTP `200` 和 `success: true`；账号或密码错误返回 HTTP `401`
  和 `success: false`；数据库不可用或查询错误返回 HTTP `503` 和
  `database_unavailable` / `database_error`。
