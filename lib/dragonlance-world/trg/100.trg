#10000
dog transform~
0 f 100
~
%load% m 10055
%purge% %self%
~
#10001
frenzydog transform~
0 n 100
~
wait 30
%echo% The @GSnake@n-Dog, @rhowls@n in agony, writhing on the ground, it's skin begins to split apart. The beasts lips curl back, and its gums burst open, huge elongated @Yyellow@n canine teeth begin to tear their way through its jaw. hunching up, the dog begins to gag on its tongue. With a final @rbloodied cough@n, its @Rtongue@n flops to the ground, twisting and warping into something else.....
wait 40
%echo% @R%self.name%@n @rStands up, skin falling from it's body. Rippling muscles flex and tighten, the hunk of flesh that once was a tongue crawls up the dogs leg, and wraps itself about it's body@n
wait 30
%echo% A @rhowl@n cracks the silence of the @GSnake@n-dogs death, standing defiantly a @RFrenzied@n-dog begins to growl.
~
#10002
apprentice transform~
0 f 100
~
%load% m 10060
%purge% %self%
~
#10003
experiment resurection~
0 n 100
~
wait 10
%echo% The apprentice's face contorts in agony as his life flows away from him.
wait 20
%echo% Suddenly the body begins to twitch and jerk rapidly. The corpse begins to glow an unhealthy reddish color and the limbs and torso begin to shrivel up, until it becomes an amorphous mass of flesh and claws struggling to stand up.
wait 30
%echo% %self.name% stands up, no longer a peaceful creature, but a raving monster ready to attack.
~
#10004
teacher death~
0 f 100
~
%load% m 10061
%purge% %self%
~
#10005
Peryton transforms~
0 n 100
~
wait 10
%echo% With one las glower the teacher looks at you as if he wanted to continue the fight. Then he falls to the ground, dead.
wait 20
%echo% The corpse starts glowing an unhealthy reddish color and the contour starts becoming blurry and changing.  Wings come out of its sides, talons form from the legs, antlers develop from its head and the whole of its body starts getting covered with feathers and fur in places.
wait 30
%echo% @r %self.name% @n flaps its wings and takes to flight, but immediatley turns to strike.
~
#10006
Paceful Druid's death~
0 f 100
~
%load% m 10000
%purge% %self%
~
#10007
Evil Druid's death~
0 f 100
~
%load% m 10001
%purge% %self%
~
#10008
Pseudodragon Blue transformation~
0 n 100
~
wait 10
%echo% The Druid gives you a reproachful look and then his eyes close. He exclaims "Sylvanus, I'm coming to you" and lays still.
wait 20
%echo% A reddish glow surrounds the corpse. Its outline changing, growing, extending until it becomes somthing enourmous and scaly. Its color starts changing into a deep sapphire @bBlue@n and its starts growing a tremendous tail.
wait 20
%echo% With a growl and a snap of its jaws a huge@B %self.name% @nturns and scans the area. Evil eyes look for its next meal.
~
#10009
Pseudodragon Green transformation~
0 n 100
~
wait 10
%echo% The Druid laugh a manical laugh falls to the ground dead.
wait 20
%echo% A bluish glow surrounds the corpse. Its outline changing, growing, extending until it becomes somthing enourmous and scaly. Its color starts changing into a deep emerald @gGreen@n and its starts growing a tremendous tail.
wait 20
%echo% With a growl and a snap of its jaws a huge@G %self.name% @nturns and scans the area. Its eyes look around for the person that inflicted it so much pain.
~
#10010
Good Renthor's death~
0 f 100
~
%load% m 10063
%purge% %self%
~
#10011
Banelar's resurrection~
0 n 100
~
wait 50
%echo% The HighDruid is @BDeath@n and the world seems to be at peace.
wait 50
@rSuddenly the corpse starts glowing a@Rred hue@r. The corpse decomposes at an accelerated rate and from what appeared to be a worm that was eating it starts sprouting tentacles and growing, and growing...
wait 60
%echo% @R %self.name% @n slithers away from the corpse and starts casting enchantments on itself. This monster does not look like a pushover.
~
#10012
Baneguard's Death~
0 f 100
~
%load% m 10066
%purge% %self%
~
#10013
Djinni resurrect~
0 n 100
~
wait 10
%echo% The body of the dead baneguard starts blurring, and begins to @bshimmer and fade@n into something else.
wait 20
%echo% It begins to grow at an accelerated rate and takes on muscles and skin. It still its humanoid in apperance but much larger than a human.
wait 20
%echo% With a jump a @B%self.name%@n takes to the air and gets surrounded by a whirlwind. Debry starts flying around the room while two blue eyes peer from the middle of the mini-tornado.
~
#10014
Evil Renthor's death~
0 f 100
~
%load% m 10068
%load% m 10069
%purge% %self%
~
#10015
Entrance of Renthor Overlord~
0 n 100
~
wait 10
%echo% @RRenthor@y falls to the ground and lays dead at your feet.
wait 30 
%echo% The corpse then starts laughing and stands up again. The @Rred shimmer@y it used to have has changed to a @Mpurple@y. He looks at you amused and waves his hand.
wait 20 
%echo% @yA HUGE @DPhaerlin Giant@y appears from nowhere and looks at his master awaiting his orders.
~
#10050
Paying for Ferry and transporting~
0 m 100
~
say Thank you, step aboard
wait 2
%echoaround% %actor% %self.name% pushes %actor.name% on a narrow plank onto the ferry.
%send% %actor% %self.name% helps you through the plank onto the plataform on the ferry.
wait 5
%echo% With a loud creak, the %self.name% pushes the ferry out of the port with a pole and starts the trip.
wait 20
%send% %actor% The ferry's is very fast and the trip seems uneventful.
wait 20
%send% %actor% Finally port is reached at Luskan.
wait 5
%send% %actor% %self.name% says "Thank you and come again".
%teleport% %actor% 3216
~
#10051
Paying for Ferry and transporting to Chult~
0 m 100
~
say Thank you, step aboard
wait 2
%echoaround% %actor% %self.name% pushes %actor.name% on a narrow plank onto the ferry.
%send% %actor% %self.name% helps you through the plank onto the plataform on the ferry.
wait 5
%echo% With a loud creak, the %self.name% pushes the ferry out of the port with a pole and starts the trip.
wait 20
%send% %actor% The ferry's is very fast and the trip seems uneventful.
wait 20
%send% %actor% Finally port is reached at Chult.
wait 5
%send% %actor% %self.name% says "Thank you and come again".
%teleport% %actor% 10070
~
#10052
Chultechos~
2 ab 100
~
eval line %random.6%
   switch %line%
     case 1
       wait 2000 s
       %zoneecho% %self.vnum% @GA really large @Binsect @Gflees as if pursued.. by what? @n
       break
case 2
       wait 2000 s
       %zoneecho% %self.vnum% @gYou have the distinct impression that you are not @Calone@n.
       break
     case 3
wait 600 s
       %zoneecho% %self.vnum% @Wa loud @gcrash@W startles you@n.
       break
     case 4
       wait 2000 s
       %zoneecho% %self.vnum% @yUnfriendly @Reyes@y look at you from several directions@n.
       break
case 5
       wait 2000 s
       %zoneecho% %self.vnum% @gA branch snaps nearby.@n
       break
     default
       wait 2000 s
       %zoneecho% %self.vnum% @ySomething @rlarge@y crashes through the underbrush@n.
     break
~
$~
