# Welcome to d202PF Codebase

This is the Pathfinder/d20 Mudbase.  It is based off of Gicker's Forgotten Realms mud which is based off CircleMud 3.1.  This is an attempt to create a stable Pathfinder/d20 mud which utilizes many of the pathfinder features.  Some are more difficult to implement, but are still there in some fashion.  For those who follow Gicker, his Luminari mud is far more advanced, but this also focuses on being a more modular MUD being able to toggle switches to be able to choose which rules the code follows.

# Credits

The original credits for CircleMud 3.1 can be found in the doc directory as license.txt (or license.pdf).

# Update as of DEC-14-2023:

I am getting back to work on this code.  These are the following things I am working on:

[ ] Getting the code to compile cleanly.  This means addressing some long-standing "warnings" the compiler gives about the code.

[ ] Fixing and documenting the SQL tables the code uses.  When it was given to me, the tables were setup for me.  That information has been lost however and I'm going to rebuild the SQL tables and document it.  If possible, I'll build it into the code to create the tables if the tables are not already there.

[ ] Work through some of the other things documented under issues in GitHub.

[ ] Get the code in good enough condition to make sure it's able to be run down the road by whomever.  Right now, it 'works', but could be better and that's the goal.

# MYSQL Setup

## Create Database

In order to run this at maximum effectiveness, you will need to create a MYSQL database called 'd202pf'.  In structs.h, you can change the name of the database as well as the username and password for the database.  If you running this on a host, you will also need to set it to their SQL server location.

## Create 'player_data' table

This is the code for creating the player_data table.
```
CREATE TABLE player_data (
    idnum INT PRIMARY KEY,
    online INT,
    name VARCHAR(255),
    title VARCHAR(255),
    titlenocolor VARCHAR(255),
    rp_points INT,
    deity VARCHAR(255),
    laston DATETIME,
    artisan_exp FLOAT,
    experience INT,
    classes VARCHAR(255),
    race VARCHAR(255),
    quest_points INT,
    clan VARCHAR(255),
    clan_rank VARCHAR(255),
    web_password VARCHAR(255),
    alignment VARCHAR(255),
    level INT,
    account VARCHAR(255),
    adm_level INT,
    clan_rank_num INT
);
```