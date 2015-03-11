#4500
book~
2 c 6
pull~
if %cmd.mudcommand% == pull && book /= %arg%
return 0
else
 %send% %actor% @YYou examine the instructions scrawled across the table:
 %send% %actor% "Don't forget your book.  Take it with you to the treetops"
 %send% %actor%  You reach over and pick up a book encased in animal skin.@n
wait 1 sec
%load% obj 4504 %actor%
%send% %actor% @RYou have found: "  A book entitled: Nocturn of the Woods"@n
end
~
#4501
pull bushes~
2 c 100
pull~
if ("%arg%" == "bushes")
wsend %actor% You pull back the bushes, revealing a secret path to the south!
wechoaround %actor% As %actor.name% pulls the bushes, a secret path is revealed to the south!
wdoor 4544 south room 4545
else
wsend %actor% Pull what?
end
~
#4502
bushes reset~
2 f 100
~
%door% 4544 south purge
~
#4503
wield axe~
0 n 100
~
* No Script
~
#4504
Enter Woodcutters Room~
0 gi 100
~
%echo% This trigger commandlist is not complete!
~
$~
