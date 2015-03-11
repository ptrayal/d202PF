/* Database Interface File */
/* Created for gates of krynn mud */
/* by. Jason Ragsdale (2002) */


//extern MYSQL *conn;
const char        *mySQL_host;
const unsigned int mySQL_port;
const char        *mySQL_socket;
const char        *mySQL_user;
const char        *mySQL_pass;
const char	  *mySQL_player_table;
const char        *mySQL_spell_table;
const char        *mySQL_aff_table;
const char        *mySQL_feat_table;
const char	  *mySQL_skill_table;
const char	  *mySQL_log_table;
const char	  *mySQL_help_table;
const char	  *mySQL_social_table;
const char	  *mySQL_intro_table;
const char	  *mySQL_mail_table;
const char	  *mySQL_rent_table;
const char	  *mySQL_rent_obj_table;;

/* minimum level a player must be to send mail	*/
#define MIN_MAIL_LEVEL 2
/* Maximum size of mail in bytes (arbitrary)	*/
#define MAX_MAIL_SIZE 100000

#define	SAVE_LOG 	1
#define GODCMD_LOG	2

#define MSG_UNREAD	1
#define MSG_READ	2
#define MSG_SAVE	3

/* Local Defines */
#define MOD_INSERT      1
#define MOD_DELETE      2
#define MOD_RETRIEVE    3

#define SQL_INSERT      1
#define SQL_REPLACE     2
#define SQL_DELETE      3
