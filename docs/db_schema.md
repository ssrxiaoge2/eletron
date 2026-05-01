# Database Schema

数据库使用 MySQL 8.0.46，字符集统一为 `utf8mb4`。初始化脚本位于 `sql/init.sql`，所有业务表均包含 `created_at` 和 `updated_at`。

## users

用户账号表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 用户主键，自增 |
| username | VARCHAR(64) | 登录用户名，唯一 |
| display_name | VARCHAR(64) | 展示昵称 |
| password_hash | VARCHAR(255) | 密码哈希 |
| avatar_url | VARCHAR(512) | 头像地址，可为空 |
| status | ENUM | `active` / `disabled` / `deleted` |
| last_login_at | DATETIME | 最近登录时间 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

## friendships

好友关系表。通过 `user_low_id` 和 `user_high_id` 生成列保证任意两个用户之间只有一条关系记录。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 关系主键，自增 |
| requester_id | BIGINT UNSIGNED | 发起用户 ID |
| addressee_id | BIGINT UNSIGNED | 接收用户 ID |
| user_low_id | BIGINT UNSIGNED | 两个用户 ID 中较小值，生成列 |
| user_high_id | BIGINT UNSIGNED | 两个用户 ID 中较大值，生成列 |
| status | ENUM | `pending` / `accepted` / `rejected` / `blocked` |
| requested_at | DATETIME | 申请时间 |
| accepted_at | DATETIME | 通过时间 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

## messages

单聊消息表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 消息主键，自增 |
| sender_id | BIGINT UNSIGNED | 发送用户 ID |
| receiver_id | BIGINT UNSIGNED | 接收用户 ID |
| content | TEXT | 消息内容 |
| content_type | ENUM | `text` / `image` / `file` / `system` |
| status | ENUM | `sent` / `delivered` / `read` / `revoked` |
| sent_at | DATETIME | 发送时间 |
| delivered_at | DATETIME | 送达时间 |
| read_at | DATETIME | 已读时间 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

## sessions

登录会话表。代码只存储令牌哈希，不保存明文令牌。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 会话主键，自增 |
| user_id | BIGINT UNSIGNED | 所属用户 ID |
| token_hash | VARCHAR(255) | 访问令牌哈希，唯一 |
| refresh_token_hash | VARCHAR(255) | 刷新令牌哈希，唯一，可为空 |
| client_info | VARCHAR(255) | 客户端信息 |
| ip_address | VARCHAR(45) | IPv4 或 IPv6 地址 |
| expires_at | DATETIME | 过期时间 |
| revoked_at | DATETIME | 吊销时间 |
| last_seen_at | DATETIME | 最近活跃时间 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
