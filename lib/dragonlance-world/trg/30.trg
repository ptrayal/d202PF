#3000
Luskan Credit Award - Level 3 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 6
remote luskan_credits %actor.id%
else
set luskan_credits 6
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 6 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3001
Luskan Credit Award - Level 4 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 10
remote luskan_credits %actor.id%
else
set luskan_credits 10
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 10 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3002
Luskan Credit Award - Level 5 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 15
remote luskan_credits %actor.id%
else
set luskan_credits 15
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 15 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3003
Luskan Credit Award - Level 6 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 21
remote luskan_credits %actor.id%
else
set luskan_credits 21
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 21 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3004
Luskan Credit Award - Level 1 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 1
remote luskan_credits %actor.id%
else
set luskan_credits 1
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 1 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3005
Luskan Credit Award - Level 2 Mob~
0 f 100
~
if (%actor.varexists(luskan_credits)%)
eval luskan_credits %actor.luskan_credits% + 3
remote luskan_credits %actor.id%
else
set luskan_credits 3
remote luskan_credits %actor.id%
end
%send% %actor% @yYou have gained 3 credits with the Luskan Justice Department.  Please see the official in the Luskan Justice Department Office in the Palace Square of Luskan to redeem your credits.@n
%send% %actor% @yYour new credit total is %actor.luskan_credits% credits.@n
~
#3006
Crafting Shop Stocking Script~
0 bi 1
~
* For creating crafting shop inventory
* 100% chance of loading steel
%load% obj 64000
* 25% chance of loading a minor magical essence
if (%random.4% == 1)
%load% obj 64100
end
* 25% chance of loading an obsidian
if (%random.4% == 1)
%load% obj 64001
end
* 25% chance of loading cold iron
if (%random.4% == 1)
%load% obj 64002
end
* 20% chance of loading an onyx
if (%random.5% == 1)
%load% obj 64003
end
* 50% chance of loading silver
if (%random.2% == 1)
%load% obj 64004
end
* 30% chance of loading gold
if (%random.10% <= 3)
%load% obj 64005
end
* 15% chance of loading a ruby
if (%random.100% <= 15)
%load% obj 64006
end
* 20% chance of loading mithril
if (%random.5% == 1)
%load% obj 64007
end
* 10% chance of loading a sapphire
if (%random.10% == 1)
%load% obj 64008
end
* 10% chance of loading an emerald
if (%random.10% == 1)
%load% obj 64009
end
* 10% chance of loading adamantine
if (%random.10% == 1)
%load% obj 64010
end
* 5% chance of loading a diamond
if (%random.20% == 1)
%load% obj 64011
end
if (%random.4% == 1)
%load% obj 64105
end
if (%random.4% == 1)
%load% obj 64110
end
if (%random.4% == 1)
%load% obj 64015
end
if (%random.4% == 1)
%load% obj 64120
end
if (%random.4% == 1)
%load% obj 64125
end
if (%random.5% == 1)
%load% obj 64106
end
if (%random.5% == 1)
%load% obj 64111
end
if (%random.5% == 1)
%load% obj 64020
end
if (%random.5% == 1)
%load% obj 64021
end
if (%random.5% == 1)
%load% obj 64126
end
if (%random.7% == 1)
%load% obj 64107
end
if (%random.7% == 1)
%load% obj 64112
end
if (%random.7% == 1)
%load% obj 64117
end
if (%random.7% == 1)
%load% obj 64022
end
if (%random.7% == 1)
%load% obj 64127
end
if (%random.8% == 1)
%load% obj 64108
end
if (%random.8% == 1)
%load% obj 64113
end
if (%random.8% == 1)
%load% obj 64118
end
if (%random.8% == 1)
%load% obj 64023
end
if (%random.8% == 1)
%load% obj 64128
end
if (%random.10% == 1)
%load% obj 64109
end
if (%random.10% == 1)
%load% obj 64014
end
if (%random.10% == 1)
%load% obj 64119
end
if (%random.10% == 1)
%load% obj 64124
end
if (%random.10% == 1)
%load% obj 64129
end
if (12.4% == 1)
%load% obj 64016
end
%load% obj 64014
%load% obj 64015
%load% obj 64016
if (12.4% == 1)
%load% obj 64016
end
if (12.5% == 1)
%load% obj 64021
end
if (12.5% == 1)
%load% obj 64022
end
if (12.10% == 1)
%load% obj 64023
end
~
#3045
stableshop~
0 c 100
*~
if %cmd.mudcommand% == list
  *
  %send% %actor%
  %send% %actor%  ##   Pet                                                 Cost
  %send% %actor% --------------------------------------------------------------
  %send% %actor%   1)  a Wolf       (level 4)                              14000
  %send% %actor%   2)  a apaloosa   (level 8)                              5000
  %send% %actor%   3)  a mule       (level 6)                              2000
  *
elseif %cmd.mudcommand% == buy
  if %actor.gold% < 1500
    tell %actor.name% You have no money, go beg somewhere else.
    halt
    * lets not allow them to have more than one pet to keep the game balanced.
  elseif %actor.follower%
    tell %actor.name% You already have someone following you
    halt
  end
  if horse /= %arg% || %arg% == 1
    set pet_name wolf
    set pet_vnum 4079
    set pet_cost 14000
  elseif fine /= %arg% || mare /= %arg% || %arg% == 2
    set pet_name apaloosa
    set pet_vnum 4080
    set pet_cost 5000
  elseif stallion /= %arg% || %arg% == 3
    set pet_name mule
    set pet_vnum 4081
    set pet_cost 2000
  else
    tell %actor.name% What? I don't have that.
    halt
  end
  *
  if %actor.gold% < %pet_cost% 
tell %actor.name% You don't have enough gold for that.
  else
    * Need to load the mob, have it follow the player AND set the affect
    * CHARM so the mob will follow the masters orders.
    %load% mob %pet_vnum%
    %force% %pet_name% mfollow %actor%
    dg_affect %pet_name% charm on 999
    emote opens the stable door and returns leading your animal by its reins.
    tell %actor.name% here you go. Treat'em well.
    nop %actor.gold(-%pet_cost%)
  end
elseif %cmd.mudcommand% == sell
  tell %actor.name% Does it look like I buy things?
else
  return 0
end
~
$~
