CREATE DATABASE IF NOT EXISTS im_app
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE im_app;

SET FOREIGN_KEY_CHECKS = 0;
DROP TABLE IF EXISTS sessions;
DROP TABLE IF EXISTS messages;
DROP TABLE IF EXISTS friendships;
DROP TABLE IF EXISTS users;
SET FOREIGN_KEY_CHECKS = 1;

CREATE TABLE users (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(32) NOT NULL UNIQUE,
  nickname VARCHAR(32) DEFAULT '',
  password_hash VARCHAR(256) NOT NULL,
  avatar VARCHAR(512) DEFAULT '',
  signature VARCHAR(128) DEFAULT '',
  email VARCHAR(64) DEFAULT '',
  status TINYINT DEFAULT 0 COMMENT '0 offline 1 online 2 invisible',
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted TINYINT(1) DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE friendships (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id BIGINT UNSIGNED NOT NULL,
  friend_id BIGINT UNSIGNED NOT NULL,
  status TINYINT DEFAULT 0 COMMENT '0 pending 1 accepted 2 rejected',
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted TINYINT(1) DEFAULT 0,
  UNIQUE KEY uk_friendships_pair (user_id, friend_id),
  KEY idx_friendships_user_id (user_id),
  KEY idx_friendships_friend_id (friend_id),
  CONSTRAINT fk_friendships_user_id FOREIGN KEY (user_id) REFERENCES users(id),
  CONSTRAINT fk_friendships_friend_id FOREIGN KEY (friend_id) REFERENCES users(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE messages (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  sender_id BIGINT UNSIGNED NOT NULL,
  receiver_id BIGINT UNSIGNED NOT NULL,
  content TEXT NOT NULL,
  type TINYINT DEFAULT 0 COMMENT '0 text 1 image 2 file',
  is_read TINYINT(1) DEFAULT 0,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted TINYINT(1) DEFAULT 0,
  KEY idx_messages_sender_id (sender_id),
  KEY idx_messages_receiver_id (receiver_id),
  KEY idx_messages_created_at (created_at),
  CONSTRAINT fk_messages_sender_id FOREIGN KEY (sender_id) REFERENCES users(id),
  CONSTRAINT fk_messages_receiver_id FOREIGN KEY (receiver_id) REFERENCES users(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE sessions (
  id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id BIGINT UNSIGNED NOT NULL,
  token_hash VARCHAR(256) NOT NULL UNIQUE,
  expired_at DATETIME NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted TINYINT(1) DEFAULT 0,
  KEY idx_sessions_user_id (user_id),
  KEY idx_sessions_expired_at (expired_at),
  CONSTRAINT fk_sessions_user_id FOREIGN KEY (user_id) REFERENCES users(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

INSERT INTO users (username, nickname, password_hash, avatar, signature, email, status)
VALUES ('testuser', '测试用户', SHA2('123456', 256), '/avatars/default.png', '用于前端登录验证的初始化用户', '', 0);

INSERT INTO users (username, nickname, password_hash, avatar, signature, email, status)
VALUES
  ('user1', '测试用户1', SHA2('123456', 256), '', '今天也要好好聊天', 'user1@example.com', 1),
  ('user2', '东方-Askeai', SHA2('123456', 256), '', '今天也要好好聊天', 'user2@example.com', 0);

SET @user1_id = (SELECT id FROM users WHERE username = 'user1' LIMIT 1);
SET @user2_id = (SELECT id FROM users WHERE username = 'user2' LIMIT 1);

INSERT INTO messages (sender_id, receiver_id, content, type, is_read, created_at)
VALUES
  (@user2_id, @user1_id, '你好，MainWindow 终于开始像一个聊天窗口了。', 0, 0, '2024-01-01 15:27:00'),
  (@user1_id, @user2_id, '先用 Mock 数据把 UI 骨架跑通，后面再接 WebSocket。', 0, 1, '2024-01-01 15:28:00'),
  (@user2_id, @user1_id, '现在后端开始返回真实聊天记录。', 0, 0, '2024-01-01 15:29:00'),
  (@user1_id, @user2_id, '收到，我这边会按接口替换 Mock 数据。', 0, 1, '2024-01-01 15:30:00'),
  (@user2_id, @user1_id, '今天晚上一起聊', 0, 0, '2024-01-01 15:31:00');
