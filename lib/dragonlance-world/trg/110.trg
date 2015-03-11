#11001
Tunnelecho~
2 b 100
~
eval line %random.6%
   switch %line%
     case 1
       wait 2 s
       %zoneecho% %self.vnum% A cold wind sweeps through from the north.
       break
case 2
       wait 2 s
       %zoneecho% %self.vnum% The grunt of a large troll can be heard nearby.
       break
     case 3
       wait 6 s
       %zoneecho% %self.vnum% You hear a strange sound.
       break
     case 4
       wait 2 s
       %zoneecho% %self.vnum% You can smell burning flesh nearby.
       break
case 5
       wait 2 s
       %zoneecho% %self.vnum% A branch snaps nearby.
       break
     default
       wait 2 s
       %zoneecho% %self.vnum% A chilly breeze from the north sends shivers through you.
     break
~
$~
