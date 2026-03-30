/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"

extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

void weather_and_time(int mode);
void another_hour(int mode);
void weather_change(void);
int determine_sky_change(int current_sky);
void apply_weather_change(int change, int old_sky);

/* External functions from weather_tracking.c */
extern void update_zone_weather_tracking(struct zone_data *zone);
extern int calculate_base_temperature(void);


void weather_and_time(int mode)
{
    another_hour(mode);
    if (mode)
    {
        weather_change();
    }
}


void another_hour(int mode)
{
    time_info.hours++;

    if (mode) {
        switch (time_info.hours) {
            case 5:
            weather_info.sunlight = SUN_RISE;
            send_to_outdoor("The sun rises in the east.\r\n");
            break;
            case 6:
            weather_info.sunlight = SUN_LIGHT;
            send_to_outdoor("The day has begun.\r\n");
            break;
            case 21:
            weather_info.sunlight = SUN_SET;
            send_to_outdoor("The sun slowly disappears in the west.\r\n");
            break;
            case 22:
            weather_info.sunlight = SUN_DARK;
            send_to_outdoor("The night has begun.\r\n");
            break;
            default:
            break;
        }
    }
if (time_info.hours > 23) {	/* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 30) {
        time_info.day = 0;
        time_info.month++;

        if (time_info.month > 12) {
            time_info.month = 0;
            time_info.year++;
        }
    }
}
}


void weather_change(void)
{
   int diff, change = 0;
   int old_sky = weather_info.sky;
   zone_rnum i;

   /* Update pressure (keep existing logic) */
   if ((time_info.month >= 9) && (time_info.month <= 16)) {
      diff = (weather_info.pressure > 985 ? -2 : 2);
   } else {
      diff = (weather_info.pressure > 1015 ? -2 : 2);
   }

   weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));
   weather_info.change = MIN(weather_info.change, 12);
   weather_info.change = MAX(weather_info.change, -12);
   weather_info.pressure += weather_info.change;
   weather_info.pressure = MIN(weather_info.pressure, 1040);
   weather_info.pressure = MAX(weather_info.pressure, 960);

   /* Update temperature */
   weather_info.temperature = calculate_base_temperature();

   /* Update wind - influenced by pressure changes */
   if (abs(weather_info.change) > 8) {
      weather_info.wind_speed += dice(2, 10);
   } else if (abs(weather_info.change) > 4) {
      weather_info.wind_speed += dice(1, 5);
   } else {
      weather_info.wind_speed -= dice(1, 3); // Wind dies down
   }
   weather_info.wind_speed = MIN(weather_info.wind_speed, 100);
   weather_info.wind_speed = MAX(weather_info.wind_speed, 0);

   /* Update humidity */
   if (weather_info.sky == SKY_RAINING || weather_info.sky == SKY_LIGHTNING) {
      weather_info.humidity = MIN(weather_info.humidity + 10, 100);
   } else {
      weather_info.humidity = MAX(weather_info.humidity - 5, 30);
   }

   /* Update fog based on humidity and temperature */
   if (weather_info.humidity > 90 && weather_info.temperature < 50) {
      weather_info.fog_level = MIN(weather_info.fog_level + 2, 10);
   } else {
      weather_info.fog_level = MAX(weather_info.fog_level - 1, 0);
   }

   /* Determine precipitation type based on temperature */
   if (weather_info.temperature < 32) {
      weather_info.precipitation_type = PRECIP_SNOW;
   } else if (weather_info.temperature < 35) {
      weather_info.precipitation_type = PRECIP_SLEET;
   } else {
      weather_info.precipitation_type = PRECIP_RAIN;
   }

   /* Update sky conditions */
   change = determine_sky_change(old_sky);

   /* Apply weather change and send messages */
   apply_weather_change(change, old_sky);

   /* Update all zone weather tracking data */
   /* Only update if zones have been loaded */
   if (zone_table && top_of_zone_table >= 0) {
      for (i = 0; i <= top_of_zone_table; i++) {
         update_zone_weather_tracking(&zone_table[i]);
      }
   }
}

/* Separate function for sky changes - keeps existing logic */
int determine_sky_change(int current_sky)
{
   int change = 0;

   switch (current_sky) {
      case SKY_CLOUDLESS:
         if (weather_info.pressure < 990)
            change = 1;
         else if (weather_info.pressure < 1010)
            if (dice(1, 4) == 1)
               change = 1;
         break;

      case SKY_CLOUDY:
         if (weather_info.pressure < 970)
            change = 2;
         else if (weather_info.pressure < 990) {
            if (dice(1, 4) == 1)
               change = 2;
            else
               change = 0;
         } else if (weather_info.pressure > 1030)
            if (dice(1, 4) == 1)
               change = 3;
         break;

      case SKY_RAINING:
         if (weather_info.pressure < 970) {
            if (dice(1, 4) == 1)
               change = 4;
            else
               change = 0;
         } else if (weather_info.pressure > 1030)
            change = 5;
         else if (weather_info.pressure > 1010)
            if (dice(1, 4) == 1)
               change = 5;
         break;

      case SKY_LIGHTNING:
         if (weather_info.pressure > 1010)
            change = 6;
         else if (weather_info.pressure > 990)
            if (dice(1, 4) == 1)
               change = 6;
         break;

      default:
         change = 0;
         break;
   }

   return change;
}

/* Apply weather changes and send appropriate messages */
void apply_weather_change(int change, int old_sky)
{
   const char *rain_msgs[] = {
      "A light drizzle begins to fall.",
      "It starts to rain.",
      "Dark clouds gather and rain begins to fall.",
      "Rain begins to patter against the ground."
   };

   const char *snow_msgs[] = {
      "Snowflakes begin to drift down from the sky.",
      "It starts to snow.",
      "Large snowflakes begin to fall, quickly covering the ground.",
      "Snow begins to fall steadily."
   };

   const char *sleet_msgs[] = {
      "Icy rain begins to fall.",
      "Sleet starts to pelt down from the sky."
   };

   switch (change) {
      case 0:
         break;

      case 1:
         send_to_outdoor("The sky starts to get cloudy.\r\n");
         weather_info.sky = SKY_CLOUDY;
         break;

      case 2:
         if (weather_info.precipitation_type == PRECIP_SNOW)
            send_to_outdoor("%s", snow_msgs[rand_number(0, 3)]);
         else if (weather_info.precipitation_type == PRECIP_SLEET)
            send_to_outdoor("%s", sleet_msgs[rand_number(0, 1)]);
         else
            send_to_outdoor("%s", rain_msgs[rand_number(0, 3)]);
         send_to_outdoor("\r\n");
         weather_info.sky = SKY_RAINING;
         break;

      case 3:
         send_to_outdoor("The clouds disappear.\r\n");
         weather_info.sky = SKY_CLOUDLESS;
         break;

      case 4:
         send_to_outdoor("Lightning starts to show in the sky.\r\n");
         send_to_outdoor("Thunder rumbles ominously overhead.\r\n");
         weather_info.sky = SKY_LIGHTNING;
         break;

      case 5:
         if (weather_info.precipitation_type == PRECIP_SNOW)
            send_to_outdoor("The snow stops.\r\n");
         else if (weather_info.precipitation_type == PRECIP_SLEET)
            send_to_outdoor("The sleet stops.\r\n");
         else
            send_to_outdoor("The rain stops.\r\n");
         weather_info.sky = SKY_CLOUDY;
         break;

      case 6:
         send_to_outdoor("The lightning stops.\r\n");
         weather_info.sky = SKY_RAINING;
         break;
   }
}
