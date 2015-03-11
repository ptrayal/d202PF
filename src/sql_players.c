


#include "constants.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

void mysql_save_char(struct char_data *ch, int loadroom) {

  MYSQL *conn = NULL;
  MYSQL_RES *res = NULL;
  MYSQL_ROW *row = NULL;

   int idnum = GET_IDNUM(ch);

  char *server = "localhost";
  char *user = "web";
  char *password = "cowboys";
  char *database = "portaldb";

  char query[MAX_STRING_LENGTH];
  char logmsg[MAX_STRING_LENGTH];

  char prfbuf1[MAX_STRING_LENGTH];
  char prfbuf2[MAX_STRING_LENGTH];
  char prfbuf3[MAX_STRING_LENGTH];
  char prfbuf4[MAX_STRING_LENGTH];

  char plrbuf1[MAX_STRING_LENGTH];
  char plrbuf2[MAX_STRING_LENGTH];
  char plrbuf3[MAX_STRING_LENGTH];
  char plrbuf4[MAX_STRING_LENGTH];

  char affbuf1[MAX_STRING_LENGTH];
  char affbuf2[MAX_STRING_LENGTH];
  char affbuf3[MAX_STRING_LENGTH];
  char affbuf4[MAX_STRING_LENGTH];

  sprintascii(prfbuf1, PRF_FLAGS(ch)[0]);
  sprintascii(prfbuf2, PRF_FLAGS(ch)[1]);
  sprintascii(prfbuf3, PRF_FLAGS(ch)[2]);
  sprintascii(prfbuf4, PRF_FLAGS(ch)[3]);

  sprintascii(plrbuf1, PLR_FLAGS(ch)[0]);
  sprintascii(plrbuf2, PLR_FLAGS(ch)[1]);
  sprintascii(plrbuf3, PLR_FLAGS(ch)[2]);
  sprintascii(plrbuf4, PLR_FLAGS(ch)[3]);

  sprintfascii(affbuf1, AFF_FLAGS(ch)[0]);  
  sprintfascii(affbuf2, AFF_FLAGS(ch)[1]);  
  sprintfascii(affbuf3, AFF_FLAGS(ch)[2]);  
  sprintfascii(affbuf4, AFF_FLAGS(ch)[3]);
  
  if (loadroom < 1)
    loadroom = GET_LOADROOM(ch);

  conn = mysql_init(NULL);

  // Connect to database

  if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
    log("MYSQL: Error connecting to database on mysql_save_char().\r\n");
    return;
  }

  // Send mysql query

  sprintf(query, "SELECT * FROM player_data WHERE idnum = '%ld'", idnum);

//  send_to_char(query, ch);

  mysql_query(conn, query);

  res = mysql_use_result(conn);

  if (res != NULL)
    row = mysql_fetch_row(res);

  if (row == NULL) {
    sprintf(query, "INSERT INTO player_data "
      "(idnum,name,alias,description,short_description,title,poofin,poofout,passwd,host,birth,"
      "played,last_logon,hometown,weight,height,sex,class,race,char_level,strength,stradd,intelligence,"
      "wisdom,dexterity,constitution,charisma,mana,max_mana,move,max_move,hit,max_hit,armor,bank_gold,"
      "exp,quest_points,hitroll,damroll,platinum,steel,gold,copper,alignment,ethos,"
      "practices,wimpy,freeze_lvl,invis_lvl,loadroom,prf_flags,bad_pws,approved,olc_zone,"
      "deity,rp_percent,aff_flags,plr_flags,pfilepos,clan,clan_lvl,money"
                   ") VALUES("
      "'%ld','%s','%s','%s','%s','%s','%s','%s','%s','%s','%d',"
      "'%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d',"
      "'%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d',"
      "'%d','%d','%d','%d','%d','%d','%d','%d','%d','%d',"
      "'%d','%d','%d','%d','%d','%s','%d','%d','%d',"
      "'%d','%d','%s','%s','%d','%d','%d','%d','%d','%d','%d','%d',"
      "'%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d',"
      "'%d','%s','%s','%s','%s','%s','%s','%s','%s','%s','%d','%d',"
      "'%d','%d','%d','%d','%d','%d','%d','%d',
                   ")",

     idnum, GET_NAME(ch), (ch)->player_specials->keywords, ch->player_specials->description, ch->player_specials->short_descr,
     GET_TITLE(ch), POOFIN(ch), POOFOUT(ch), GET_PASSWD(ch), GET_HOST(ch), ch->time.birth,
     ch->time.played, ch->time.logon, GET_HOME(ch), GET_WEIGHT(ch), GET_HEIGHT(ch),
     GET_SEX(ch), GET_CLASS(ch), GET_RACE(ch), GET_LEVEL(ch), GET_STR(ch), GET_ADD(ch), GET_INT(ch),
     GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch), GET_MANA(ch), GET_MAX_MANA(ch), GET_MOVE(ch),
     GET_MAX_MOVE(ch), GET_HIT(ch), GET_MAX_HIT(ch), GET_AC(ch), GET_BANK_GOLD(ch),
     GET_EXP(ch), GET_QUESTPOINTS(ch), GET_HITROLL(ch), GET_DAMROLL(ch), GET_MONEY(ch).platinum,
     GET_MONEY(ch).steel, GET_MONEY(ch).gold, GET_MONEY(ch).copper, GET_ALIGNMENT(ch), GET_ETHIC_ALIGNMENT(ch),
     GET_PRACTICES(ch), GET_WIMP_LEV(ch), GET_FREEZE_LEV(ch), GET_INVIS_LEV(ch),  loadroom,
     prfbuf1, GET_BAD_PWS(ch), GET_APPROVED(ch), GET_OLC_ZONE(ch),
     GET_GODSELECT(ch), GET_RPFACTOR(ch), affbuf1, plrbuf1, GET_PFILEPOS(ch), GET_CLAN(ch),
     GET_CLANLEVEL(ch), GET_MONEY(ch), GET_TRAINS(ch), ch->size, GET_RAGE(ch), GET_RESEARCH_TOKENS(ch),
     GET_TURN_UNDEAD(ch), GET_STRENGTH_OF_HONOR(ch), GET_ADMLEVEL(ch), GET_CLASS_LEVEL(ch), GET_LEVEL_STAGE(ch),
     GET_HITDICE(ch), GET_LEVEL_ADJ(ch), GET_COMPANION_VNUM(ch), GET_FAMILIAR_VNUM(ch), GET_MOUNT_VNUM(ch), GET_PET_VNUM(ch),
     ch->time.maxage, ch->time.created, prfbuf2, prfbuf3, prfbuf4, plrbuf2, plrbuf3, plrbuf4, affbuf2, affbuf3, affbuf4,
     GET_SAVE_BASE(ch, 0), GET_SAVE_BASE(ch, 1), GET_SAVE_BASE(ch, 2), GET_SAVE_MOD(ch, 0), GET_SAVE_MOD(ch, 1),
     GET_SAVE_MOD(ch, 2), GET_POWER_ATTACK(ch), GET_FEAT_POINTS(ch), GET_EPIC_FEAT_POINTS(ch), GET_RACE_PRACTICES(ch),

     );

  }
  else {
    sprintf(query, "UPDATE player_data SET "
      "idnum = '%ld',name = '%s',alias = '%s',description = '%s',short_description = '%s',title = '%s',"
      "poofin = '%s',poofout = '%s',passwd = '%s',host = '%s',birth = '%d',"
      "played = '%d',last_logon = '%d',hometown = '%d',weight = '%d',height = '%d',sex = '%d',class = '%d',"
      "race = '%d',char_level = '%d',strength = '%d',stradd = '%d',intelligence = '%d',"
      "wisdom = '%d',dexterity = '%d',constitution = '%d',charisma = '%d',mana = '%d',max_mana = '%d',"
      "move = '%d',max_move = '%d',hit = '%d',max_hit = '%d',armor = '%d',bank_gold = '%d',"
      "exp = '%d',quest_points = '%d',hitroll = '%d',damroll = '%d',platinum = '%d',steel = '%d',gold = '%d',"
      "copper = '%d',alignment = '%d',ethos = '%d',wimpy = '%d',"
      "practices = '%d',wimpy = '%d',freeze_lvl = '%d',invis_lvl = '%d',loadroom = '%d',prf_flags = '%d',"
      "bad_pws = '%d',approved = '%d',olc_zone = '%d',"
      "deity = '%d',rp_percent = '%d',aff_flags = '%d',plr_flags = '%d',pfilepos = '%d',clan = '%d',"
      "clan_lvl = '%d',money = '%d'"
                    " WHERE idnum = '%ld'", 
     idnum, ch->player.name, (ch)->player.name, ch->player.description, ch->player.short_descr,
     GET_TITLE(ch), POOFIN(ch), POOFOUT(ch), GET_PASSWD(ch), ch->desc->host, ch->player.time.birth,
     ch->player.time.played, ch->player.time.logon, GET_HOME(ch), GET_WEIGHT(ch), GET_HEIGHT(ch),
     GET_SEX(ch), GET_CLASS(ch), GET_RACE(ch), GET_LEVEL(ch), GET_STR(ch), GET_ADD(ch), GET_INT(ch),
     GET_WIS(ch), GET_DEX(ch), GET_CON(ch), GET_CHA(ch), GET_MANA(ch), GET_MAX_MANA(ch), GET_MOVE(ch),
     GET_MAX_MOVE(ch), GET_HIT(ch), GET_MAX_HIT(ch), GET_AC(ch), GET_BANK_GOLD(ch),
     GET_EXP(ch), GET_QUESTPOINTS(ch), GET_HITROLL(ch), GET_DAMROLL(ch), GET_MONEY(ch).platinum,
     GET_MONEY(ch).steel, GET_MONEY(ch).gold, GET_MONEY(ch).copper, GET_ALIGNMENT(ch), GET_ETHOS(ch),
     GET_PRACTICES(ch), GET_WIMP_LEV(ch), GET_FREEZE_LEV(ch), GET_INVIS_LEV(ch), loadroom,
     PRF_FLAGS(ch), GET_BAD_PWS(ch), GET_APPROVED(ch), ch->player_specials->saved.olc_zone,
     GET_GODSELECT(ch), GET_RPFACTOR(ch), AFF_FLAGS(ch), PLR_FLAGS(ch), GET_PFILEPOS(ch), GET_CLAN(ch),
     GET_CLANLEVEL(ch), GET_MONEY(ch), idnum);

  }

  // Release from memory
  if (res != NULL)
    mysql_free_result(res);

//  send_to_char(query, ch);
  
  mysql_query(conn, query);

  mysql_close(conn);

}

