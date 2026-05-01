# Changelog

## 2026-05-01

- 新增 MySQL 初始化脚本 `sql/init.sql`，包含 `users`、`friendships`、`messages`、`sessions` 四张表。
- 新增 Qt/C++ 数据库连接封装 `backend/core/Database.h` 和 `backend/core/Database.cpp`，通过 INI 配置读取 MySQL 连接信息。
- 新增数据库结构说明 `docs/db_schema.md`。
