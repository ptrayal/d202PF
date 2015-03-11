#4000
knight~
0 g 50
0~
%echo% a Knight Captain shouts: "%actor.name% prepare yourself! the Necro Kings army is on the move!
~
#4001
pull branches~
2 c 0
pull~
if (%arg% == branches)
%door% 193 north room 4600
%echo% You pull the branches and reveal a hidden path to the north.
wait 300
%door% 193 north purge
%door% 4600 south purge
%echo% The pathway to the north suddenly becomes lost in the underbrush.
else
return 0
end
~
#4002
jump~
2 c 100
jump~
%send% %actor.name% You close your eyes and leap across to the mountain below you.
%echoaround% %actor.name% As %actor.name% Jumps, you can see a look of terror in his eyes.
  %teleport% %actor.name% 193
      %echoaround% %actor.name% %actor.name% has arrived
~
#4003
room emotes for Freedom City~
2 0 100
~
* No Script
~
#4004
eye offer~
2 d 100
yuzag vigoe~
if %actor.has_item(4001)%
    wait 4s
    wsend %actor% As you speak the words, a soft glowing light begins to radiate from the shrine.
    wechoaround %actor% As %actor.name% speaks, their hand reaches forward placing the stone within the eye socket. As this happens, the light flares up brightly and they are gone...
    wait 1s
    wteleport %actor% 4399
wsend %actor% Moments pass and you finally step forward, leaving whence you came. All around you armor and weapons of legend lie. An altar to the north has a Sword atop its surface giving off a faint magical glow.
    wforce %actor% look
else
    wait 4s
    wsend %actor% A deep rumble can be herd from within the earth, and begins to speak" Find he who sits in the shadows, for he has what has been stolen".
  end
~
#4005
keypop~
2 g 100
~
if %self.contents.vnum% != 4002
  %load% obj 4002
end
%echoaround% A tiny Brass key falls to the ground from the counter.
%send% %actor% As you enter you see a tiny Brass key fall to the ground.
~
#4006
cutwood~
0 j 100
~
if %object.type% == TREASURE 
  wait 1 sec 
  emote begins swinging %object.shortdesc% at the tree 
  wait 1 sec 
  emote picks up a branch and throws it to  %actor.name%. 
  if %object.vnum% == 4005 
    wait 1 sec 
    %load% obj 4004 %actor%   
 end 
  %purge% %object% 
else 
  wait 1 s 
  drop %object.name.car% 
end
~
#4007
axeoffer~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4004)%
return 0
else
if (%actor.varexists(freedomquest_1_1)% && %actor.freedomquest_1_1% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_1 completed 
remote freedomquest_1_1 %actor.id%
%echo% %actor.name% Hands the Carpenter the Oak Branch.
%send% %actor% You hand the branch to the carpenter.
%purge% %actor.item(4004)%
wait 10
%echo% The Carpenter uses a Carpenters Plane to trim up the branch.
%echo% The Carpenter hands %actor.name% a Plank of Wood!
%load% obj 4007 %actor%
 nop %actor.exp(5000)%
  nop %actor.gold(500)%
%send% %actor% @YYou have gained Experience!@n
end
end
~
#4008
plankoffer~
0 j 100
~
if %actor.has_item(4005)%
return 0
else
  %echo% %actor.name% Hands the Carpenter the Oak Branch.
%send% %actor% You hand the branch to the carpenter.
  %purge% %object%
  wait 1 second
    %echo% The Carpenter uses a Carpenters Plane to trim up the branch.
    %echo% The Carpenter hands %actor.name% a Plank of Wood!
    %load% obj 4007 %actor%
end
~
#4009
carpentersays~
0 g 100
~
 Say You dont happen to have an axe i could use do you?
~
#4010
plankandshield~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4007)%
return 0
else
if (%actor.varexists(freedomquest_1_2)% && %actor.freedomquest_1_2% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_2 completed 
remote freedomquest_1_2 %actor.id%
%echo% %actor.name% Hands the shield Maker a plank of wood.
%send% %actor% You hand the Plank of wood to the shield maker.
%purge% %actor.item(4007)%
wait 10
%echo% The Shield Maker steps back into the back of the smithy, where curses and the pouding of hammers can be herd.
%echo% The Shield Maker steps back out and hands %actor.name% a small wooden Shield!
%load% obj 4019 %actor%
 nop %actor.exp(2500)%
  nop %actor.gold(2500)%
%send% %actor% @YYou have gained Experience!@n
end
end
~
#4011
questspeech~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
  %send% %actor% @YThe painter is covered in paint 
 %send% %actor% and surrounded by shields on the ground. I am not
 %send% %actor% the average artisan.I have found my medium to not 
 %send% %actor% be canvas, but shields. Every artist must find their
 %send% %actor% niche and that I have done. It has paid very well.@n
else
  %send% %actor% enter what?!
end
~
#4012
shieldmakerspeech~
0 g 100
~
say You dont happen to have any Wooden Planks would ya?
~
#4013
questspeech2~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
 %send% %actor% @YThe carpenter has a very concerned look on his face.
  %send% %actor% I cant work without  wood. I need oak. The shields
 %send% %actor% were due two days ago! Raay is going to kill me. 
 %send% %actor% You wouldnt happen to have any wood would you?@n
else
%send% %actor% enter what?!
end
~
#4014
questspeech3~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
 %send% %actor% @YHello there, my name is Raay. I am in quite a bind.
 %send% %actor% The guards are in need of some sturdy shields 
 %send% %actor% but my brother has fallen behind on his work again. 
 %send% %actor% I need those planks before I could ever imagine making a shield.@n
else
%send% %actor% enter what?!
end
~
#4015
shieldoffer~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4019)%
return 0
else
if (%actor.varexists(freedomquest_1_3)% && %actor.freedomquest_1_3% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_3 completed 
remote freedomquest_1_3 %actor.id%
 %echo% @YThe painter begins to coat the Shield in paint, after a few minutes he hands you %echo% dripping wet shield that looks like the fangs of a Dragon.
%echo% The Painter hands %actor.name% a@n @RPainted wooden Shield@n!
%purge% %actor.item(4019)%
wait 1
%load% obj 4018 %actor%
 nop %actor.exp(2000)%
  nop %actor.gold(500)%
%send% %actor% @YYou have gained Experience!@n
end
end
~
#4016
paintedshieldoffer~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4018)%
return 0
else
if (%actor.varexists(freedomquest_1_4)% && %actor.freedomquest_1_4% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_4 completed 
remote freedomquest_1_4 %actor.id%
%send% %actor%You hand over the painted wooden shield 
%send% %actor%and the guard nods respectfully to you.
%send% %actor% I thank you, if I was caught not having my shield 
%send% %actor% I would have been fined by my superior.  
%send% %actor% I wish I could give you something to repay you.
%send% %actor%  With that the guard quickly looks around and spots something on the ground.
%send% %actor% He spots a childs forgotten @Rmarble@n. Oh hereitit is..it is a marble. 
%send% %actor% With a smirk the guard says, Every good deed must be repaid.
%purge% %actor.item(4018)%
wait 1
%load% obj 4017 %actor%
 nop %actor.exp(100000)%
  nop %actor.gold(1500)%
%send% %actor% @YYou have gained Experience!@n
end
end
~
#4017
questspeech4~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
%send% %actor% @YA hulking figure stands before you fashioning the finest of armor.
%send% %actor% Upon his armor the sigil of the City of Latos is placed. 
%send% %actor% He holds a sword in left hand, but his right seems empty to you.
%send% %actor% The Guard turns and says, I need my shield. 
%send% %actor% I was promised my shield by Raay two days ago.@n
else
%send% %actor% enter what?!
end
~
#4018
questspeech5~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
 %send% %actor% @YYou walk up to the wall and notice a perfect circle
 %send% %actor% about the size of a marble.@n
else
%send% %actor% enter what?!
end
~
#4019
marbleoffer~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4017)%
return 0
else
if (%actor.varexists(freedomquest_1_5)% && %actor.freedomquest_1_5% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_5 completed 
remote freedomquest_1_5 %actor.id%
%send% %actor% @YYou place the @Rmarble@n @Yinto the hole in the wall
%send% %actor% and hear it travel down within the wall. 
%send% %actor% You hear a sudden click and a brick from the wall slowly opens up.
%send% %actor% As you examine the compartment you spot a@n @ysmall medallion@n
%purge% obj 4017 %actor%
wait 1
%load% obj 4016 %actor%
 nop %actor.exp(350000)%
  nop %actor.gold(1500)%
%send% %actor% @YYou have gained Experience!@n
end
end
~
#4020
questspeech6~
2 c 100
sh~
if %cmd.mudcommand% == show && quest /= %arg%
%send% %actor% @RThe northern assassin is watching
%send% %actor% the changing of the guards with all too much concentration.@n 
%send% %actor% @YI need to find a way to get passed the guards. 
%send% %actor% I have tried many disguises and dressed like them, but it seems 
%send% %actor% I am missing one thing. I lack their medallion. With a medallion 
%send% %actor% I could surely get in and finish my job.@n
else
%send% %actor% enter what?!
end
~
#4021
medallion offer~
2 c 100
offer~
if (%arg% == offer) & %actor.has_item(4016)%
return 0
else
if (%actor.varexists(freedomquest_1_6)% && %actor.freedomquest_1_6% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_6 completed 
remote freedomquest_1_6 %actor.id%
 %send% %actor% @YThe shady characters eyes light up and exclaims, 
 %send% %actor% That is it, I can surely get in now Here I have no use for this.
 %send% %actor% The last man I worked for ordered me to dispose of 
 %send% %actor% someone and wanted it to be messy. Not my style but the money was right.@n
%purge% %actor.item(4016)%
wait 1
%load% obj 4015 %actor%
 nop %actor.exp(250000)%
  nop %actor.gold(1500)%
%send% %actor% @YYou have gained Experience!@
~
#4022
wandtrigger~
1 c 7
forgrim!~
if %cmd.mudcommand% == Forgrim! /= %arg%
  %send% %actor% You Chant the rites of passage.
  %echoaround% %actor% %actor.name% Begins chanting
  %echoaround% %actor% %actor.name% shimmers a moment as the ground parts and they are sucked into a hole.
  %teleport% %actor% 4397
  nop %actor.pos(sleeping)%
  %at% 97 %echoaround% %actor% %actor.name% appears in the middle of the room laying on the floor unconscious.
else
  %send% %actor% %cmd% what?!
end
~
#4023
butcheroffer~
2 c 100
offer~
if (%actor.varexists( freedomquest_1_6)% && %actor.freedomquest_1_6% == completed)
if (%arg% == offer) & %actor.has_item(4015)%
return 0
else
if (%actor.varexists(freedomquest_1_7)% && %actor.freedomquest_1_7% == completed)
 %send% %actor% You already did that quest!
 halt
return 0
else
set freedomquest_1_7 completed 
remote freedomquest_1_7 %actor.id%
 %send% %actor% @YYou hand the butcher the cleaver and he nods with satisfaction.
 %send% %actor% Perhaps this  one wont break as easily, it looks well crafted.
 %send% %actor% Ill tell you whatThere  is a dog that has been hanging around 
 %send% %actor% here all day looking for scraps. I cant let my customers think 
 %send% %actor% this place is dirty. Take him off my hands.@n 
%purge% %actor.item(4016)%
wait 1
%load% obj 4021 %actor%
 nop %actor.exp(250000)%
  nop %actor.gold(25000)%
%send% %actor% @YYou have gained Experience!@
else
You have not completed the previous part of this quest yet
end
~
#4024
restore~
1 c 7
lor~
if %cmd.mudcommand% == Lord protect me /= %arg%
  %send% %actor% You chant the words to Forgrim, and the visage of War appears.
  %echoaround% %actor% %actor.name% Screams a Warcry as Forgrim Bestows life upon him.
  %echoaround% %actor% %actor.name% Screams For @RWAR!!@n
  %damage% %actor% -250
  else
  %send% %actor% %cmd% what?!
end
~
#4033
new trigger~
2 c 100
warcry~
%send% %actor.name% You close your eyes and scream a War cry that could Shake the Heavens.
%echoaround% %actor.name% The Drums of War begin to pound.
  %teleport% %actor.name% 1
      %echoaround% %actor.name% %actor.name% has arrived through a portal of fire and flames, the screams of the dying can be herd just as the portal closes.
~
#4034
beggartrig~
0 g 75
~
%echo% a beggar says: "%actor.name% @BAhms for the poor?"
~
#4035
new trigger~
0 c 100
~
* No Script
~
#4036
rennor transform~
0 f 100
~
%load% m 4043
%purge% %self%
~
#4037
abomtransform~
0 n 100
~
wait 20
%echo% @RRennor@n, the Mad @GDruid@n Screams in agony, writhing on the ground, his skin begins to split apart, blood pours out across the ground instantly congealing, and reforming into something else...
wait 50
%echo% @r%self.name% Stands up, skin falling from it's body.@n Rippling muscles flex and bend, as the beast stretches its body out. The @rAbomonation@n looks around slowly, taking in it's surroundings.
wait 50
%echo% a glutteral sound escapes the @rBeasts@n maw: "@RRrrreennthor@n".
~
#4038
weaponenchant~
1 b 50
~
if %self.worn_by%
  set actor %self.worn_by%
  if %actor.fighting%
    eval dam %random.6% + 10
    %damage% %actor.fighting% %dam%
%echo% A   weapon glows bright @Rred@n, and @rengulfs@n @Y%actor.fighting.name%@n in @rflames@n for @R%dam% damage.@n
 end
end
~
#4039
disctrigger~
2 d 100
grant me passage~
if %actor.has_item(4062)%
    wait 4s
    wsend %actor% As you speak the words, a soft glowing light begins to radiate from the Crystal Rose.
    wechoaround %actor% As %actor.name% speaks, holding the Crystal Rose up into the air, the light flares up brightly and they are gone...
    wait 1s
    wteleport %actor% 59902
wsend %actor% Moments pass and you finally step forward, leaving whence you came. You are standing within the Tower of @GDisc@n.
wforce %actor% look
else
    wait 4s
    wsend %actor% Only the chosen one shall enter this Tower.
end
~
#4040
tempusroom~
2 d 100
tempus~
    wsend %actor% As you speak the words, a soft glowing light begins to radiate from the walls.
    wechoaround %actor% As %actor.name% chants the words, a light envenopes them and they disapear into a bright ball of flames.
    wait 1s
    wteleport %actor% 4397
wsend %actor% Moments pass and you finally step forward, leaving whence you came. You are standing within the @rWar@n Room of @RTempus@n.
    wforce %actor% look
~
$~
