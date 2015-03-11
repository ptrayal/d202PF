char highscorename[200];

SPECIAL(dartboard)
{
  struct char_data *to;
  int dart1 = 0, dart2 = 0, dart3 = 0, score = 0;
  if (!highscorename) strcpy(highscorename, "Nobody");

  if (CMD_IS("playdarts")) {
      dart1 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_)value(ch, SKILL_DARTS);
      dart2 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_value(ch, SKILL_DARTS);
      dart3 = dice(10, 5) + (dice(1, GET_DEX(ch)) * 5) + get_skill_value(ch, SKILL_DARTS);
      if (affected_by_spell(ch, SPELL_BLESS)) {
        dart1 += 5;
        dart2 += 5;
        dart3 += 5;
      }
      score = dart1 + dart2 + dart3;
      if ((score > highscore) && (GET_ADMLEVEL(ch) == 0)) {
        highscore = score;
        strcpy(highscorename, GET_NAME(ch));
      }
      sprintf(buf, "You throw a dart and score %d!\r\n", dart1);
      send_to_char(ch, buf);
      to = world[IN_ROOM(ch)].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf, "%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart1);
          send_to_char(to, buf);
        }
      }

      sprintf(buf, "You throw a dart and score %d!\r\n", dart2);
      send_to_char(ch, buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart2);
          send_to_char(to, buf);
        }
      }

      sprintf(buf, "You throw a dart and score %d!\r\n", dart3);
      send_to_char(ch, buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"%s throws a dart and scores a %d!\r\n", PERS(ch, to), dart3);
          send_to_char(to, buf);
        }
      }

      sprintf(buf, "Your total score for this game is %d.\r\n\r\n", score);
      send_to_char(ch, buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"The total score for %s this game is %d!\r\n", PERS(ch, to), score);
          send_to_char(to, buf);
        }
      }

      sprintf(buf, "The highest score is %d held by %s.\r\n\r\n", highscore, highscorename);
      send_to_char(ch, buf);
      to = world[ch->in_room].people;
      for (; to; to = to->next_in_room) {
        if (to != ch) {
          sprintf(buf,"The highest score is held by %s at %d.\r\n", highscorename, highscore);
          send_to_char(to, buf);
        }
      }      


      if (dice(1, 100) <= 1) {
        GET_BASE_SKILL(ch, SKILL_DARTS)++;
        send_to_char("Your skill at the game of darts has improved!\r\n", ch);
      }
      return (1);
  }
  else
    return (0);
}

