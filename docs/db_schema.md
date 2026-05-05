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
| group_id | BIGINT UNSIGNED | 好友所在分组 ID，`NULL` 表示未分组，默认归入“我的好友”展示 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |

索引：`idx_friendships_user_id`、`idx_friendships_friend_id`、`idx_friendships_group_id`、`uk_friendships_pair`。

## friend_groups

好友分组表。每个用户注册后自动创建默认分组“我的好友”。
| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 分组主键，自增 |
| user_id | BIGINT UNSIGNED | 分组归属用户 ID，外键到 `users.id` |
| name | VARCHAR(32) | 分组名称 |
| sort_order | INT | 排序权重，越小越靠前 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

索引：`idx_user`、`uk_friend_groups_user_name`。

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

## files

文件/图片元数据表。文件二进制内容保存到后端本地 `backend/storage/`，数据库仅保存路径和访问元数据。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 文件主键，自增 |
| uploader_id | BIGINT UNSIGNED | 上传者用户 ID，外键到 `users.id` |
| receiver_id | BIGINT UNSIGNED | 接收者用户 ID，外键到 `users.id` |
| group_id | BIGINT UNSIGNED | 群组 ID，群文件时有值，私聊文件为空 |
| file_name | VARCHAR(256) | 原始文件名 |
| stored_name | VARCHAR(256) | 服务端 UUID 重命名后的文件名 |
| file_size | BIGINT UNSIGNED | 文件字节数 |
| file_type | TINYINT | 文件类型：1 图片，2 普通文件 |
| mime_type | VARCHAR(128) | MIME 类型 |
| storage_path | VARCHAR(512) | 服务端绝对存储路径 |
| thumbnail_path | VARCHAR(512) | 图片缩略图路径，普通文件为空 |
| is_deleted | TINYINT(1) | 软删除标记 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

索引：`idx_files_uploader`、`idx_files_receiver`、`idx_files_group`。

## conversations

会话表。用于记录用户点击好友后创建的会话入口，避免数据库层产生重复会话。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 会话主键，自增 |
| user_id | BIGINT UNSIGNED | 当前用户 ID，外键到 `users.id` |
| target_user_id | BIGINT UNSIGNED | 会话目标用户 ID，外键到 `users.id` |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |
| is_deleted | TINYINT(1) | 软删除标记 |

索引：`uk_conversations_user_target`、`idx_conversations_user_id`、`idx_conversations_target_user_id`。

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
## groups

群组基础信息表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 群组主键，自增 |
| name | VARCHAR(64) | 群名称 |
| owner_id | BIGINT UNSIGNED | 群主用户 ID |
| announcement | TEXT | 群公告 |
| member_count | INT UNSIGNED | 当前成员数 |
| max_member | INT UNSIGNED | 最大成员数 |
| avatar | VARCHAR(512) | 群头像 |
| is_dissolved | TINYINT(1) | 是否已解散 |
| created_at | DATETIME | 创建时间 |
| updated_at | DATETIME | 更新时间 |

索引：`idx_owner`。

## group_members

群成员关系表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 主键，自增 |
| group_id | BIGINT UNSIGNED | 群组 ID |
| user_id | BIGINT UNSIGNED | 成员用户 ID |
| role | TINYINT | 0 群员，1 管理员，2 群主 |
| is_muted | TINYINT(1) | 是否禁言 |
| mute_until | DATETIME | 禁言到期时间 |
| joined_at | DATETIME | 入群时间 |
| is_deleted | TINYINT(1) | 是否退出群聊 |

索引：`uk_group_user`、`idx_group`、`idx_user`。

## group_messages

群消息表。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| id | BIGINT UNSIGNED | 主键，自增 |
| group_id | BIGINT UNSIGNED | 群组 ID |
| sender_id | BIGINT UNSIGNED | 发送者用户 ID |
| content | TEXT | 消息内容 |
| type | TINYINT | 0 文字，1 图片，2 文件 |
| is_deleted | TINYINT(1) | 软删除标记 |
| created_at | DATETIME | 创建时间 |

索引：`idx_group_time`、`idx_sender`。
