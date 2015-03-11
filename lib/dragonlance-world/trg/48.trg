#4800
Drider scout~
0 f 100
~
%load% m 4898
%purge% %self%
~
#4801
Drider Assassin~
0 n 100
~
if %actor.level%<25
  say You will not give away my position!
  say DIE %actor.name%!!
mob kill %actor%
else
if %actor.level%>25
  say You should leave now %actor.name% before I kill you!!
  say Consider this a warning!!
set stunned %actor.hitp%
%damage% %actor% %stunned%
%send% %actor% You have been shot in the ribs by a tiny dart!
%send% %actor% Your body begins to twitch and spasm uncontrollably!
%send% %actor% Your veins feel like they are on fire as the poison courses through your body!
%send% %actor% You will survive but for now you should rest, that was a very close call!
 end
end
~
#4802
Drow Scout 2~
0 l 35
~
%echo% @CThe drow scout screams in agony as his body begins to @wbend@n and @wcontrot@n against his will!@n
wait 3 s
%echo% @CThe drow scouts body begins to change and form into the shape of a @Ys@Dp@yi@Yd@De@yr@C!@n
wait 2
%echo% %self.name% say @WMy life for you lady @DLloth@C!@n
~
#4807
miner block~
2 g 100
~
if %actor.is_pc%
  %echoaround% %actor% The mine is ahead.
else
  %teleport% %actor% 4803
end
~
#4890
treasury block~
2 q 100
~
if %direction% == down
  %send% %actor% You try to go %direction% but the Elite Guards block your way.
  return 0
end
~
#4892
testtrigforman~
0 q 20
stone~
if %cmd.mudcommand% == stone && is && life /= %arg%
  * findmob checks if the mob is in the room.
  if %findmob.<room vnum>(<mob vnum>)%
    %send% %actor% The Mine Foreman blocks you.
    %echoaround% %actor% As %actor.name% tries to go east, the foreman shouts halt!
  else
    %send% %actor% You chant the words and are given entrance.
    %echoaround% %actor% %actor.name% Chants something to the guard.
wteleport %actor% 4892
  end
else
  * If it doesn't match let the command continue.
  return 0
end
~
#4893
East Blocking Foreman~
2 q 100
~
if %direction% == east
  %send% %actor% You try to go %direction% but The @WForeman@n blocks your way.
  return 0
end
~
$~
