-- d202pf_master_schema.sql
-- Reconstructed master schema for the d202PF MUD database.
-- All table and column definitions are derived from SQL usage in the
-- codebase (see src/*.c).  Types and constraints have been chosen to
-- match how the game reads and writes data and should be reviewed before
-- production use.

SET NAMES utf8mb4;
SET time_zone = '+00:00';

/* ------------------------------------------------------------------ */
/* Core character snapshot data (see limits.c, clan.c, account.c).    */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_data` (
  `idnum` BIGINT UNSIGNED NOT NULL,
  `online` TINYINT(1) NOT NULL DEFAULT 0,
  `name` VARCHAR(80) NOT NULL,
  `title` VARCHAR(160) NOT NULL DEFAULT '',
  `titlenocolor` VARCHAR(160) NOT NULL DEFAULT '',
  `rp_points` INT NOT NULL DEFAULT 0,
  `deity` VARCHAR(80) NOT NULL DEFAULT '',
  `laston` DATETIME DEFAULT NULL,
  `artisan_exp` DOUBLE NOT NULL DEFAULT 0,
  `experience` BIGINT NOT NULL DEFAULT 0,
  `classes` VARCHAR(255) NOT NULL DEFAULT '',
  `race` VARCHAR(80) NOT NULL DEFAULT '',
  `quest_points` INT NOT NULL DEFAULT 0,
  `clan` VARCHAR(80) NOT NULL DEFAULT '',
  `clan_rank` VARCHAR(80) NOT NULL DEFAULT '',
  `web_password` VARCHAR(255) NOT NULL DEFAULT '',
  `alignment` VARCHAR(40) NOT NULL DEFAULT '',
  `level` INT NOT NULL DEFAULT 0,
  `account` VARCHAR(80) NOT NULL DEFAULT '',
  `adm_level` INT NOT NULL DEFAULT 0,
  `clan_rank_num` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`idnum`),
  UNIQUE KEY `uniq_player_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Forum experience synchronisation (limits.c, interpreter.c).        */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_forum_data` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `account` VARCHAR(80) NOT NULL,
  `forum_exp` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uniq_forum_account` (`account`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Login history (act.wizard.c).                                      */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_logins` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `player_name` VARCHAR(80) NOT NULL,
  `account_name` VARCHAR(80) NOT NULL DEFAULT '',
  `last_login` DATETIME NOT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_player_logins_name` (`player_name`),
  KEY `idx_player_logins_last_login` (`last_login`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Per-level announcements (interpreter.c display_levelup_changes).   */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_levels` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `idnum` BIGINT UNSIGNED NOT NULL,
  `char_name` VARCHAR(80) NOT NULL,
  `char_level` INT NOT NULL,
  `char_class` VARCHAR(80) NOT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `idx_player_levels_character` (`idnum`, `char_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Extra profile data and web experience (interpreter.c, informative).*/
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_extra_data` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(80) NOT NULL,
  `background_story` MEDIUMTEXT,
  `personality` MEDIUMTEXT,
  `background_length` INT NOT NULL DEFAULT 0,
  `personality_length` INT NOT NULL DEFAULT 0,
  `extra_account_exp` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uniq_extra_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Account email management (interpreter.c).                          */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_emails_d20` (
  `account_name` VARCHAR(80) NOT NULL,
  `email` VARCHAR(255) NOT NULL,
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Game note system (note.c, modify.c).                               */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_note_categories` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(80) NOT NULL,
  `display` VARCHAR(120) NOT NULL,
  `adm_level` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uniq_note_category` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_note_messages` (
  `id_msg` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `cat_name` VARCHAR(80) NOT NULL,
  `subject` VARCHAR(255) NOT NULL,
  `message` MEDIUMTEXT NOT NULL,
  `poster_name` VARCHAR(80) NOT NULL,
  `poster_admin_level` INT NOT NULL DEFAULT 0,
  `poster_clan` INT NOT NULL DEFAULT 0,
  `time_stamp` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id_msg`),
  KEY `idx_note_messages_cat` (`cat_name`),
  CONSTRAINT `fk_note_messages_category`
    FOREIGN KEY (`cat_name`) REFERENCES `player_note_categories` (`name`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_note_read` (
  `id_msg` INT UNSIGNED NOT NULL,
  `player_name` VARCHAR(80) NOT NULL,
  `cat_name` VARCHAR(80) NOT NULL,
  `read_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id_msg`, `player_name`),
  KEY `idx_note_read_cat` (`cat_name`),
  CONSTRAINT `fk_note_read_message`
    FOREIGN KEY (`id_msg`) REFERENCES `player_note_messages` (`id_msg`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* In-game mail system (new_mail.c, modify.c).                        */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `player_mail` (
  `mail_id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `sender` VARCHAR(80) NOT NULL,
  `receiver` VARCHAR(80) NOT NULL,
  `subject` VARCHAR(255) NOT NULL,
  `message` MEDIUMTEXT NOT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`mail_id`),
  KEY `idx_player_mail_receiver` (`receiver`),
  KEY `idx_player_mail_sender` (`sender`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_mail_read` (
  `player_name` VARCHAR(80) NOT NULL,
  `mail_id` INT UNSIGNED NOT NULL,
  `read_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_name`, `mail_id`),
  CONSTRAINT `fk_mail_read_mail`
    FOREIGN KEY (`mail_id`) REFERENCES `player_mail` (`mail_id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `player_mail_deleted` (
  `player_name` VARCHAR(80) NOT NULL,
  `mail_id` INT UNSIGNED NOT NULL,
  `deleted_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`player_name`, `mail_id`),
  CONSTRAINT `fk_mail_deleted_mail`
    FOREIGN KEY (`mail_id`) REFERENCES `player_mail` (`mail_id`)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Polling system (polls.c, act.other.c).                             */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `poll_data` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(80) NOT NULL,
  `poll_num` INT NOT NULL,
  `option` INT NOT NULL,
  `date` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uniq_poll_vote` (`name`, `poll_num`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Aggregated online statistics (act.wizard.c do_usage_stats_mysql).  */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `mud_num_online` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `num_online` INT NOT NULL DEFAULT 0,
  `num_registered` INT NOT NULL DEFAULT 0,
  `num_quests` INT NOT NULL DEFAULT 0,
  `num_rooms` INT NOT NULL DEFAULT 0,
  `snapshot_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* ------------------------------------------------------------------ */
/* Website integration tables (interpreter.c).                        */
/* ------------------------------------------------------------------ */
CREATE TABLE IF NOT EXISTS `mud_web_info` (
  `forum_user_id` INT UNSIGNED NOT NULL,
  `account_exp` INT NOT NULL DEFAULT 0,
  PRIMARY KEY (`forum_user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `jos_chronoforms_user_details` (
  `cf_user_id` INT UNSIGNED NOT NULL,
  `gameaccount` VARCHAR(80) NOT NULL,
  PRIMARY KEY (`cf_user_id`),
  UNIQUE KEY `uniq_chronoforms_gameaccount` (`gameaccount`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `jos_users` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `username` VARCHAR(150) NOT NULL,
  `email` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `uniq_users_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

