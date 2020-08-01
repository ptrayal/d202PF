#3510
luskan ruins entrance trigger~
2 c 0
enter~
if ("%arg%" == "whirlpool")
%echoaround% %actor% %actor.name% steps forward and is suddenly sucked down into the small whirlpool!
%send% %actor% As you step onto the whirlpool, you are suddenly sucked down into it by an unseen force!
%teleport% %actor% 3600
end
~
#3523
leave small tunnel echo~
2 g 100
~
if %direction% == northeast
  %echoaround% %actor% Suddenly, %actor.name% emerges from behind a small grate in the eastern wall of the tunnel!
  %send% %actor% As you crawl through the grate, you suddenly find yourself in the sewers once more.
end
~
#3533
warrior grate~
2 c 100
open~
if ("%arg%" == "grate")
%send% %actor% The grate lifts easily as you pull on it, allowing access to a smaller drainage tunnel.
%echoaround% %actor% As %actor.name% pulls on the grate, it opens effortlessly, offering access to a smaller side tunnel.
%door% 3533 south flag a
%door% 3533 south room 3579
%door% 3533 south name grate
end
~
#3534
warriors square grate close purge~
2 c 100
close~
if ("%arg%" == "grate")
%echo% The crashing sound of the grate resonates through the sewers as it is closed.
%door% 3533 south purge
end
~
#3535
hide grate exit~
2 f 100
~
%door% 3533 south purge
~
#3536
warrior grate exit purge~
2 q 100
~
if %direction% == south
  wait 1
  %door% 3533 south purge
end
~
#3545
commoner arrow ~
2 c 100
look~
if ("%arg%" == "arrow")
%send% %actor% The arrow points to the west.
end
~
#3577
writing on the wall~
2 c 100
look~
if ("%arg%" == "writing")
%send% %actor% arrow...lair...EVIL
end
~
#3579
small tunnel entrance grate close~
2 g 100
~
* No Script
~
$~
