// This deals with helping the modularization of the different campaigns.
// This helps keep all the campaign information in one place and makes it easier to change
// and look at.

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "deities.h"


// Determines what deity pantheon is used based on the campaign.
int DetermineCampaign()
{
    int campaign = -1;
    switch(CONFIG_CAMPAIGN) 
    {
        default:
        campaign = DEITY_PANTHEON_ALL;
        break;
        case CAMPAIGN_DRAGONLANCE:
        campaign = DEITY_PANTHEON_DL_PRE_CAT;
        break;
        case CAMPAIGN_GOLARION:
        campaign = DEITY_PANTHEON_GOLARION;
        break;
        case CAMPAIGN_FORGOTTEN_REALMS:
        campaign = DEITY_PANTHEON_FAERUNIAN;
        break;
    }
    return campaign;
}

// What is the name of the world based on the campaign.
const char *CampaignWorld[] = 
{
  "Unknown - Alert Admin",
  "Faerun",
  "Krynn",
  "Golarion",
  NULL
};

// What is the name of the campaign.
const char *CampaignName[] = 
{
  "Unknown - Alert Admin",
  "Forgotten Realms",
  "Dragonlance: Age of Legends",
  "Pathfinder - Golarion",
  NULL
};

// What is the maximum number of languages based on the campaign.
int CampaignMaxLanguages()
{
    int campaign = -1;
    switch(CONFIG_CAMPAIGN) 
    {
        default:
        campaign = MAX_LANGUAGES;
        break;
        case CAMPAIGN_DRAGONLANCE:
        campaign = MAX_LANGUAGES_DL_AOL;
        break;
        case CAMPAIGN_GOLARION:
        campaign = MAX_LANGUAGES;
        break;
        case CAMPAIGN_FORGOTTEN_REALMS:
        campaign = MAX_LANGUAGES_FR;
        break;
    }
    return campaign;
}

// 
//  Unstable Code below this comment.  As code becomes working, we move it above.
// 


