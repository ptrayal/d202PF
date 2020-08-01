#15200
Test of High Sorcery~
2 g 100
~
%echo% Thus begins your Test of High Sorcery
wait 4 s
%echo% Beware the way ahead is fraught with danger, you will either succeed or you will die.  There is no failure.
wait 4 s
%echo% The rules are simple:
wait 4 s
%echo%An applicant cannot escape the Testing Grounds through use of magic.
wait 4 s
%echo%An applicant cannot use magic to transport himself from one area within the Test to another.
wait 4 s
%echo%An applicant cannot use magic to contact creatures outside the Test.
wait 4 s
%echo%An applicant cannot use magic to summon creatures from Krynn to the Testing Grounds
wait 4 s
%echo% The Test Begins Now.
%teleport% %actor% 15200
%force% %actor% look
~
#15201
Low Risk~
2 c 100
low~
%echo% You have decided to take Low Risk for the next objective.
~
#15202
Average Risk~
2 c 100
average~
%echo% You have decided to take Average Risk for the next objective.
~
#15203
High Risk~
2 c 100
high~
%echo% You have decided to take High Risk for the next objective.
~
#15204
Grave Risk~
2 c 100
grave~
%echo% You have decided to take Grave Risk for the next objective.
~
#15205
HazardA~
2 g 100
~
wait 10 s
%echo% You hear a grinding sound coming from the east and west wall.
wait 6 s
%echo% The grinding sounds from the wall get louder, and you notice less available space within the room.
wait 6 s
%echo% You realize that the walls are closing in on you, you should find that exit immediately.
wait 6 s
%echo% You realize that you are running out of time as the grinding sounds get closer and closer.
~
#15206
lever pull~
2 c 100
pull~
%send% %actor% as you slide you hand into the hole you feel a brief tingling sensation.
wait 2 s
%send% %actor% you quickly pull the lever and remove your hand.
wait 2 s
%send% %actor% a secret passage opens ahead of you and you quickly step through.
wait 2 s
%teleport% %actor% 15207
%force% %actor% look
~
#15207
mend trigger~
2 p 100
cast~
      if (%spellname% = mending)
        %send% %actor% You mend the shard and a deep glow surrounds you.
        return 1
      end
~
$~
