#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "db.h"
#include "deities.h"


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

const char *CampaignWorld[] = 
{
  "Unknown - Alert Admin",
  "Faerun",
  "Krynn",
  "Golarion"
};

// 
//  Unstable Code below this comment.  As code becomes working, we move it above.
// 



