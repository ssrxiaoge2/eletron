# Database Schema

数据库使用 MySQL 8.0.46，字符集统一为 `utf8mb4`，排序规则为 `utf8mb4_unicode_ci`。初始化脚本位于 `sql/init.sql`，后端目录同步保存 `backend/sql/init.sql`。

所有业务表使用 `ENGINE=InnoDB`。外键字段均建立索引。业务删除统一使用 `is_deleted` 软删除字段，服务端不做物理删除。

## users

用户表。初始化脚本会创建测试用户：`testuser / 123456`、`user1 / 123456`、`user2 / 123456`，密码以 SHA-256 哈希存储。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 用户主键，自增 |
| username | VARCHAR(32) | 登录账号，唯一 |
| password_hash | VARCHAR(256) | SHA-256 密码哈希 |
| nickname | VARCHAR(32) | 显示昵称 |
| status | TINYINT | 在线状态：0 离线，1 在线，2 隐身 |
| avatar | VARCHAR(512) | 头像地址，默认空字符串 |
| created_at | DATETIME | 注册时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |
| gender | VARCHAR(16) | 性别 |
| birthday | DATE | 生日 |
| signature | VARCHAR(128) | 个性签名 |
| email | VARCHAR(64) | 邮箱地址，兼容前端和测试展示字段 |

## friendships

好友关系表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 好友关系主键，自增 |
| user_id | BIGINT UNSIGNED | 用户 ID，外键到 `users.id` |
| friend_id | BIGINT UNSIGNED | 好友用户 ID，外键到 `users.id` |
| status | TINYINT | 状态：0 待确认，1 已同意，2 已拒绝 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |

索引：`idx_friendships_user_id`、`idx_friendships_friend_id`、`uk_friendships_pair`。

## messages

消息表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 消息主键，自增 |
| sender_id | BIGINT UNSIGNED | 发送者 ID，外键到 `users.id` |
| receiver_id | BIGINT UNSIGNED | 接收者 ID，外键到 `users.id` |
| content | TEXT | 消息内容 |
| type | TINYINT | 消息类型：0 文字，1 图片，2 文件 |
| is_read | TINYINT(1) | 是否已读 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |

索引：`idx_messages_sender_id`、`idx_messages_receiver_id`、`idx_messages_created_at`。

## sessions

会话表。服务端只保存 JWT 的 SHA-256 哈希，不保存明文 token。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 会话主键，自增 |
| user_id | BIGINT UNSIGNED | 所属用户 ID，外键到 `users.id` |
| token_hash | VARCHAR(256) | JWT SHA-256 哈希，唯一 |
| expired_at | DATETIME | 过期时间 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |

索引：`idx_sessions_user_id`、`idx_sessions_expired_at`。
