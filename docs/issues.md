# Issues

## 2026-05-01

- 待对接：登录接口尚未写入 `docs/api.md`。前端当前临时调用
  `POST http://127.0.0.1:8000/auth/login`，请求体为
  `{ "username": "...", "password": "..." }`。期望后端返回：
  成功时 HTTP 2xx 且 `success: true`；账号或密码错误时 HTTP 401/403/400
  或 `success: false`；数据库不可用时 HTTP 503 或错误码
  `database_unavailable` / `database_error`。
