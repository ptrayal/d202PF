/* ************************************************************************
*   File: weather_tracking.c                            Part of D202 MUD *
*  Usage: Weather tracking system for Pathfinder-style tracking          *
*                                                                         *
*  Enhanced weather system with zone-specific tracking data integration  *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

/* External variables */
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct weather_data weather_info;
extern struct time_info_data time_info;
extern room_rnum top_of_world;
extern zone_rnum top_of_zone_table;

/* Function prototypes */
void init_zone_weather(struct zone_data *zone);
void auto_configure_zone_climate(struct zone_data *zone);
int get_zone_predominant_sector(struct zone_data *zone);
int get_sector_temp_modifier(int sector_type);
int calculate_base_temperature(void);
int get_zone_temperature(struct zone_data *zone);
int get_room_temperature(room_rnum room);
void update_zone_weather_tracking(struct zone_data *zone);
int calculate_tracking_dc(struct char_data *ch, struct char_data *vict);
const char *get_tracking_condition_desc(int dc);

/* Initialize weather tracking data for a zone */
void init_zone_weather(struct zone_data *zone)
{
   CREATE(zone->weather_data, struct zone_weather_data, 1);

   zone->weather_data->ground_moisture = GROUND_DRY;
   zone->weather_data->snow_state = SNOW_NONE;
   zone->weather_data->snow_depth = 0;
   zone->weather_data->rain_hours = 0;
   zone->weather_data->hours_since_rain = 100; // Dry initially
   zone->weather_data->hours_since_snow = 0;
   zone->weather_data->last_precip_time = time(0);

   /* Set climate modifiers - can be customized per zone */
   zone->weather_data->base_temp_modifier = 0;
   zone->weather_data->altitude = 0;
   zone->weather_data->storm_frequency = 50; // Average
}

/* Get predominant sector type for a zone (for climate calculations) */
int get_zone_predominant_sector(struct zone_data *zone)
{
   int sector_count[20] = {0}; // Array to count each sector type
   int max_count = 0;
   int predominant_sector = SECT_FIELD; // Default
   room_rnum i;
   int sector;

   /* Safety check: world might not be loaded yet during boot */
   if (!world || top_of_world < 0)
      return predominant_sector;

   /* Count sector types in the zone */
   for (i = 0; i <= top_of_world; i++) {
      if (world[i].zone == (zone - zone_table)) {
         sector = world[i].sector_type;
         if (sector >= 0 && sector < 20) {
            sector_count[sector]++;
            if (sector_count[sector] > max_count) {
               max_count = sector_count[sector];
               predominant_sector = sector;
            }
         }
      }
   }

   return predominant_sector;
}

/* Calculate temperature modifier based on sector type */
int get_sector_temp_modifier(int sector_type)
{
   switch (sector_type) {
      case SECT_DESERT:
         return 20;  // Hot and dry

      case SECT_TUNDRA:
         return -30; // Arctic cold

      case SECT_GLACIER:
         return -40; // Permanent ice, extremely cold

      case SECT_MOUNTAIN:
         return -15; // Cold and high altitude

      case SECT_CAVE:
         return -10; // Cool underground

      case SECT_JUNGLE:
         return 15;  // Hot and humid

      case SECT_SWAMP:
         return 5;   // Slightly warmer, humid

      case SECT_UNDERWATER:
         return -5;  // Cool water temperature

      case SECT_FLYING:
         return -20; // Very cold at altitude

      case SECT_CITY:
         return 5;   // Urban heat island effect

      case SECT_FOREST:
         return -5;  // Shaded, cooler

      case SECT_BEACH:
         return 0;   // Moderate coastal climate

      case SECT_PLAINS:
         return 0;   // Open, temperate

      case SECT_FIELD:
      case SECT_HILLS:
      case SECT_ROAD:
      case SECT_INSIDE:
      default:
         return 0;   // Temperate/neutral
   }
}

/* Auto-configure zone climate based on its rooms */
void auto_configure_zone_climate(struct zone_data *zone)
{
   int predominant_sector;

   if (!zone->weather_data)
      return;

   predominant_sector = get_zone_predominant_sector(zone);

   /* Set temperature modifier based on predominant sector */
   zone->weather_data->base_temp_modifier = get_sector_temp_modifier(predominant_sector);

   /* Set altitude for mountain/flying zones */
   if (predominant_sector == SECT_MOUNTAIN || predominant_sector == SECT_FLYING) {
      zone->weather_data->altitude = 50; // Higher elevation
   } else if (predominant_sector == SECT_GLACIER) {
      zone->weather_data->altitude = 30; // High altitude ice
   }

   /* Set storm frequency based on sector */
   if (predominant_sector == SECT_MOUNTAIN) {
      zone->weather_data->storm_frequency = 70; // More storms in mountains
   } else if (predominant_sector == SECT_SWAMP || predominant_sector == SECT_JUNGLE) {
      zone->weather_data->storm_frequency = 60; // Humid, more rain
   } else if (predominant_sector == SECT_DESERT || predominant_sector == SECT_TUNDRA) {
      zone->weather_data->storm_frequency = 20; // Dry, rare storms
   } else if (predominant_sector == SECT_BEACH) {
      zone->weather_data->storm_frequency = 55; // Coastal storms
   }
}

/* Calculate base temperature for current time/season */
int calculate_base_temperature(void)
{
   int temp = 60; // Base moderate temperature

   /* Seasonal variation (months 0-12) */
   /* Assuming: 0-2 = Winter, 3-5 = Spring, 6-8 = Summer, 9-11 = Fall */
   if (time_info.month >= 0 && time_info.month <= 2) {
      temp = 30; // Winter (Dec, Jan, Feb)
   } else if (time_info.month >= 3 && time_info.month <= 5) {
      temp = 55; // Spring (Mar, Apr, May)
   } else if (time_info.month >= 6 && time_info.month <= 8) {
      temp = 80; // Summer (Jun, Jul, Aug)
   } else if (time_info.month >= 9 && time_info.month <= 11) {
      temp = 55; // Fall (Sep, Oct, Nov)
   } else {
      temp = 30; // Month 12+ = Winter
   }

   /* Daily variation - cooler at night, warmer during day */
   if (time_info.hours >= 0 && time_info.hours <= 5) {
      temp -= 15; // Coldest before dawn
   } else if (time_info.hours >= 6 && time_info.hours <= 11) {
      temp += 5; // Warming up morning
   } else if (time_info.hours >= 12 && time_info.hours <= 17) {
      temp += 10; // Hottest afternoon
   } else if (time_info.hours >= 18 && time_info.hours <= 21) {
      temp += 0; // Cooling evening
   } else {
      temp -= 10; // Night
   }

   return temp;
}

/* Get temperature for a specific zone */
int get_zone_temperature(struct zone_data *zone)
{
   int temp = calculate_base_temperature();

   if (!zone || !zone->weather_data)
      return temp;

   /* Zone base modifier (from sector types) */
   temp += zone->weather_data->base_temp_modifier;

   /* Altitude effect: -1 degree per 1000 feet */
   temp -= (zone->weather_data->altitude / 10);

   /* Weather modifiers */
   if (weather_info.sky == SKY_RAINING || weather_info.sky == SKY_LIGHTNING) {
      temp -= 10; // Rain cools things down
   }

   if (weather_info.sky == SKY_CLOUDLESS && time_info.hours >= 12 && time_info.hours <= 17) {
      temp += 5; // Clear sunny days are hotter
   }

   /* Wind chill effect */
   if (weather_info.wind_speed > 20) {
      temp -= (weather_info.wind_speed / 10); // Wind makes it feel colder
   }

   return temp;
}

/* Get temperature for a specific room (considers sector type) */
int get_room_temperature(room_rnum room)
{
   struct zone_data *zone;
   int temp;
   int sector;

   if (room < 0 || room > top_of_world)
      return 60;

   zone = &zone_table[world[room].zone];
   temp = get_zone_temperature(zone);
   sector = world[room].sector_type;

   /* Room-specific modifiers based on sector */
   switch (sector) {
      case SECT_INSIDE:
      case SECT_CAVE:
         /* Indoor/cave rooms are insulated from weather */
         temp = 65; // Comfortable room temperature
         break;

      case SECT_UNDERWATER:
         /* Water temperature is more stable */
         temp = 45; // Cool water
         break;

      case SECT_DESERT:
         /* Desert can have extreme temperature swings */
         if (time_info.hours >= 12 && time_info.hours <= 17) {
            temp += 15; // Very hot during day
         } else if (time_info.hours >= 22 || time_info.hours <= 5) {
            temp -= 20; // Very cold at night
         }
         break;

      case SECT_FOREST:
      case SECT_JUNGLE:
         /* Forest/jungle shade moderates temperature */
         if (time_info.hours >= 12 && time_info.hours <= 17) {
            temp -= 10; // Cooler in shade
         }
         break;

      case SECT_GLACIER:
         /* Glacier is always freezing */
         temp = MIN(temp, 10); // Never above 10°F
         break;

      case SECT_TUNDRA:
         /* Tundra is very cold */
         temp = MIN(temp, 20); // Never above 20°F
         break;
   }

   return temp;
}

/* Update zone-specific weather tracking data */
void update_zone_weather_tracking(struct zone_data *zone)
{
   struct zone_weather_data *zw;
   int zone_temp;

   if (!zone || !zone->weather_data)
      return;

   zw = zone->weather_data;
   zone_temp = get_zone_temperature(zone);

   /* Update rain accumulation */
   if (weather_info.sky == SKY_RAINING || weather_info.sky == SKY_LIGHTNING) {
      zw->rain_hours++;
      zw->rain_hours = MIN(zw->rain_hours, 48); // Cap at 48 hours
      zw->hours_since_rain = 0;
      zw->last_precip_time = time(0);

      /* Ground becomes saturated or frozen */
      if (zone_temp > 32) {
         zw->ground_moisture = GROUND_SATURATED;
      } else {
         zw->ground_moisture = GROUND_FROZEN; // Frozen ground
      }

      /* Handle snow vs rain */
      if (weather_info.precipitation_type == PRECIP_SNOW) {
         zw->snow_depth += dice(1, 3); // 1-3 inches per hour
         zw->snow_depth = MIN(zw->snow_depth, 60);
         zw->snow_state = SNOW_FRESH;
         zw->hours_since_snow = 0;
      }
   } else {
      /* No precipitation - things dry out and snow ages */
      zw->hours_since_rain++;

      /* Ground moisture decay */
      if (zone_temp < 32) {
         zw->ground_moisture = GROUND_FROZEN; // Frozen
      } else if (zw->hours_since_rain > 36) {
         zw->ground_moisture = GROUND_DRY;
      } else if (zw->hours_since_rain > 18) {
         zw->ground_moisture = GROUND_MOIST;
      } else if (zw->hours_since_rain > 6) {
         zw->ground_moisture = GROUND_MUDDY;
      }

      /* Decay rain accumulation over time */
      if (zw->hours_since_rain > 48)
         zw->rain_hours = 0;

      /* Snow aging */
      if (zw->snow_depth > 0) {
         zw->hours_since_snow++;

         /* Age snow states */
         if (zw->hours_since_snow > 24) {
            zw->snow_state = SNOW_OLD;
         } else if (zw->hours_since_snow > 12) {
            zw->snow_state = SNOW_PACKED;
         } else if (zw->hours_since_snow > 2) {
            zw->snow_state = SNOW_PACKED;
         }

         /* Snow melts if temperature is above freezing */
         if (zone_temp > 32) {
            zw->snow_depth -= dice(1, 4);
            if (zw->snow_depth <= 0) {
               zw->snow_depth = 0;
               zw->snow_state = SNOW_NONE;
            }
         }

         /* Snow becomes ice if it's very old and cold */
         if (zw->hours_since_snow > 72 && zone_temp < 25) {
            zw->snow_state = SNOW_ICE;
         }
      }
   }
}

/*
 * Calculate tracking DC based on Pathfinder rules, weather, and sector type
 * Returns the DC for a Survival check to track
 */
int calculate_tracking_dc(struct char_data *ch, struct char_data *vict)
{
   room_rnum room = IN_ROOM(ch);
   struct zone_data *zone;
   struct zone_weather_data *zw;
   int dc = 15; // Default firm ground DC
   int zone_num;
   int sector;

   /* Get zone weather data */
   zone_num = world[room].zone;
   if (zone_num < 0 || zone_num > top_of_zone_table)
      return dc;

   zone = &zone_table[zone_num];
   zw = zone->weather_data;
   sector = world[room].sector_type;

   if (!zw)
      return dc;

   /* ===== BASE DC from sector type and ground conditions ===== */
   /* Per Pathfinder: Very soft=5, Soft=10, Firm=15, Hard=20 */

   /* First check for impossible tracking conditions */
   switch (sector) {
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_UNDERWATER:
         return 999; // Can't track in water

      case SECT_FLYING:
         return 999; // Can't track in air
   }

   /* Snow overrides other ground conditions */
   if (zw->snow_depth > 0 && zw->snow_state != SNOW_ICE) {
      /* Snow (not ice) is very soft ground */
      dc = 5;
   } else if (zw->snow_state == SNOW_ICE) {
      /* Ice is hard ground */
      dc = 20;
   } else {
      /* Base DC from sector type */
      switch (sector) {
         case SECT_DESERT:
            /* Sand is very soft - but only if not frozen */
            if (zw->ground_moisture == GROUND_FROZEN)
               dc = 20; // Frozen desert sand
            else
               dc = 5;  // Soft sand
            break;

         case SECT_SWAMP:
            /* Swamp is very soft */
            dc = 5;
            break;

         case SECT_BEACH:
            /* Beach sand is very soft */
            dc = 5;
            break;

         case SECT_TUNDRA:
            /* Tundra - usually frozen */
            if (zw->ground_moisture == GROUND_FROZEN)
               dc = 20;
            else
               dc = 10; // Soft frozen ground when not completely frozen
            break;

         case SECT_GLACIER:
            /* Glacier is always hard ice */
            dc = 20;
            break;

         case SECT_JUNGLE:
         case SECT_FIELD:
         case SECT_FOREST:
            /* Modified by ground moisture */
            switch (zw->ground_moisture) {
               case GROUND_SATURATED:
                  dc = 5;  // Muddy field/forest floor
                  break;
               case GROUND_MUDDY:
                  dc = 10; // Soft
                  break;
               case GROUND_MOIST:
                  dc = 15; // Firm
                  break;
               case GROUND_DRY:
                  dc = 15; // Firm
                  break;
               case GROUND_FROZEN:
                  dc = 20; // Hard frozen ground
                  break;
               default:
                  dc = 15;
            }
            break;

         case SECT_PLAINS:
            /* Open grassland - similar to field */
            switch (zw->ground_moisture) {
               case GROUND_SATURATED:
                  dc = 5;
                  break;
               case GROUND_MUDDY:
                  dc = 10;
                  break;
               default:
                  dc = 15;
            }
            break;

         case SECT_HILLS:
         case SECT_MOUNTAIN:
            /* Rocky terrain - generally firm to hard */
            if (zw->ground_moisture == GROUND_SATURATED)
               dc = 10; // Muddy mountain trails
            else if (zw->ground_moisture == GROUND_MUDDY)
               dc = 15;
            else
               dc = 20; // Rocky hard ground
            break;

         case SECT_CAVE:
            /* Cave floor depends on moisture */
            if (zw->ground_moisture == GROUND_SATURATED)
               dc = 10; // Wet cave floor
            else
               dc = 20; // Hard stone
            break;

         case SECT_CITY:
         case SECT_ROAD:
         case SECT_INSIDE:
            /* Paved/hard surfaces */
            dc = 20;
            break;

         default:
            /* Default firm ground */
            dc = 15;
            break;
      }
   }

   /* ===== MODIFIERS ===== */

   /* +1 DC per hour of rain (Pathfinder rule) */
   dc += MIN(zw->rain_hours, 48);

   /* Fresh snow covers tracks (+10 DC - Pathfinder rule) */
   if (zw->snow_state == SNOW_FRESH) {
      dc += 10;
   }

   /* Current fog or precipitation (+3 DC - Pathfinder rule) */
   if (weather_info.fog_level > 3 ||
       weather_info.sky == SKY_RAINING ||
       weather_info.sky == SKY_LIGHTNING) {
      dc += 3;
   }

   /* Visibility conditions at night (Pathfinder rule) */
   if (weather_info.sunlight == SUN_DARK || weather_info.sunlight == SUN_SET) {
      /* Only apply night penalties outdoors */
      if (sector != SECT_INSIDE && sector != SECT_CAVE) {
         /* Moonlight: +3 DC, No moon: +6 DC */
         if (weather_info.moon_phase >= 5 && weather_info.moon_phase <= 7) {
            dc += 3; // Full moon range
         } else {
            dc += 6; // Partial/new moon
         }
      }
   }

   /* Wind erodes tracks over time */
   if (weather_info.wind_speed > 40) {
      /* Severe wind: +2 DC per hour in open terrain */
      if (sector == SECT_FIELD || sector == SECT_DESERT ||
          sector == SECT_ROAD || sector == SECT_PLAINS || sector == SECT_BEACH)
         dc += (zw->hours_since_rain * 2);
   } else if (weather_info.wind_speed > 25) {
      /* Strong wind: +1 DC per 2 hours in open terrain */
      if (sector == SECT_FIELD || sector == SECT_DESERT ||
          sector == SECT_ROAD || sector == SECT_PLAINS || sector == SECT_BEACH)
         dc += (zw->hours_since_rain / 2);
   } else if (weather_info.wind_speed > 15) {
      /* Moderate wind: +1 DC per 6 hours in open terrain */
      if (sector == SECT_FIELD || sector == SECT_DESERT ||
          sector == SECT_ROAD || sector == SECT_PLAINS || sector == SECT_BEACH)
         dc += (zw->hours_since_rain / 6);
   }

   /* Forest/jungle provides protection from wind erosion */
   if ((sector == SECT_FOREST || sector == SECT_JUNGLE) && weather_info.wind_speed > 25) {
      dc -= 5; // Trees block wind
   }

   return MIN(dc, 999); // Cap at impossible
}

/* Helper function to get description of tracking conditions */
const char *get_tracking_condition_desc(int dc)
{
   if (dc >= 999)
      return "Tracking is impossible in this terrain.";
   else if (dc >= 40)
      return "Tracks are nearly impossible to discern.";
   else if (dc >= 30)
      return "Environmental conditions make tracking extremely challenging.";
   else if (dc >= 25)
      return "Weather and terrain make tracking very difficult.";
   else if (dc >= 20)
      return "The hard ground makes tracking difficult.";
   else if (dc >= 15)
      return "Tracks can be seen in the terrain.";
   else if (dc >= 10)
      return "The soft ground holds tracks well.";
   else
      return "The ground shows tracks very clearly.";
}
