#50200
Hello to Shandug caravan~
0 d 100
Hello Shandug~
bow
wait 10
say Hello my friend, I am Shandug the trader. I am on my way to sell some wares to other cities.
wait 10
say It appears that you are on your way out of town as well. Mayhaps we can help each other.
grin
wait 10
say I can provide transportation to whatever city you are going if you can help me as a guard for my caravan.
wait 10
say just name tell me the place that you intend to go and we will be on out way.
wait 10
say I usually travel between triboar, mirabar, luskan, neverwinter and waterdeep.
wait 10
say I do have to charge you 100 gold pieces for the supplies and food you'll use.
wait 10
say But if we do come across any bandits, I'll be sure to pay you for your work.
wait 10
bow
~
#50201
To Mirabar~
0 d 100
Mirabar~
if  (%actor.gold% > 99)
 %actor.gold(-100)%
 say It will be my pleasure. Step onto the camel please. Oh, and 100 gold coins please.
wait 10
      %echoaround% %actor% %actor.name% climbs on top of a dusty camel.
      %send% %actor% You climb on top of a dusty and smelly camel. 
wait 10
      %echoaround% %actor% %self.name% Hurries the caravan along the road.
      %send% %actor% %self.name% Hurries the caravan along the road.
      %teleport% %actor% 4964
wait 30
      %echoaround% %actor% %actor.name% climbs down from the top of a dusty camel.
      %send% %actor% You climb down from a dusty, smelly and now sweaty camel. Your muscles are sore.
        %send% %actor% Shandug says "Thank you, and call on me if you need to travel again."
else
 say I'm afraid you don't have enough coin on you.  I need 100 gold to cover my expenses.
end
~
#50202
To Triboar~
0 d 100
Triboar~
if  (%actor.gold% > 99)
 %actor.gold(-100)%
 say It will be my pleasure. Step onto the camel please.
wait 5
      %echoaround% %actor% %actor.name% climbs on top of a dusty camel.
      %send% %actor% You climb on top of a dusty and smelly camel. 
wait 5
      %echoaround% %actor% %self.name% Hurries the caravan along the road.
      %send% %actor% %self.name% Hurries the caravan along the road.
      %teleport% %actor% 51000
wait 10
      %echoaround% %actor% %actor.name% climbs down from the top of a dusty camel.
      %send% %actor% You climb down from a dusty, smelly and now sweaty camel. Your muscles are sore.
        %send% %actor% Shandug says "Thank you, and call on me if you need to travel again."
else
 say I'm afraid you don't have enough coin on you.  I need 100 gold to cover my expenses.
end
~
#50203
To Waterdeep~
0 d 100
Waterdeep~
if  (%actor.gold% > 99)
 %actor.gold(-100)%
 say It will be my pleasure. Step onto the camel please.
wait 5
      %echoaround% %actor% %actor.name% climbs on top of a dusty camel.
      %send% %actor% You climb on top of a dusty and smelly camel. 
wait 5
      %echoaround% %actor% %self.name% Hurries the caravan along the road.
      %send% %actor% %self.name% Hurries the caravan along the road.
      %teleport% %actor% 50781
wait 10
      %echoaround% %actor% %actor.name% climbs down from the top of a dusty camel.
      %send% %actor% You climb down from a dusty, smelly and now sweaty camel. Your muscles are sore.
        %send% %actor% Shandug says "Thank you, and call on me if you need to travel again."
else
 say I'm afraid you don't have enough coin on you.  I need 100 gold to cover my expenses.
end
~
#50204
To Neverwinter~
0 d 100
Neverwinter~
if  (%actor.gold% > 99)
 %actor.gold(-100)%
 say It will be my pleasure. Step onto the camel please.
wait 5
      %echoaround% %actor% %actor.name% climbs on top of a dusty camel.
      %send% %actor% You climb on top of a dusty and smelly camel. 
wait 5
      %echoaround% %actor% %self.name% Hurries the caravan along the road.
      %send% %actor% %self.name% Hurries the caravan along the road.
      %teleport% %actor% 50500
wait 10
      %echoaround% %actor% %actor.name% climbs down from the top of a dusty camel.
      %send% %actor% You climb down from a dusty, smelly and now sweaty camel. Your muscles are sore.
        %send% %actor% Shandug says "Thank you, and call on me if you need to travel again."
else
 say I'm afraid you don't have enough coin on you.  I need 100 gold to cover my expenses.
end
~
#50205
To Luskan~
0 d 100
Luskan~
if  (%actor.gold% > 99)
 %actor.gold(-100)%
 say It will be my pleasure. Step onto the camel please.
wait 5
      %echoaround% %actor% %actor.name% climbs on top of a dusty camel.
      %send% %actor% You climb on top of a dusty and smelly camel. 
wait 5
      %echoaround% %actor% %self.name% Hurries the caravan along the road.
      %send% %actor% %self.name% Hurries the caravan along the road.
      %teleport% %actor% 50200
wait 10
      %echoaround% %actor% %actor.name% climbs down from the top of a dusty camel.
      %send% %actor% You climb down from a dusty, smelly and now sweaty camel. Your muscles are sore.
        %send% %actor% Shandug says "Thank you, and call on me if you need to travel again."
else
 say I'm afraid you don't have enough coin on you.  I need 100 gold to cover my expenses.
end
~
#50210
Random mob generator~
2 b 100
~
eval line %random.12%
switch %line%
     case 1
        Load m 2956
        break
     case 2
        load m 2956
        break
    case 3
        load m 2956
        break
    case 4
        load m 2956
        break
    case 5
       load m 2956
       break
    case 6
       load m 2956
       break
    case 7
       load m 2956
       break
    case 8
       load m 2956
       break
    case 9
       load m 2956
       break
    case 10
       load m 2956
       break
   case 11
       load m 2956
       break
   default
       load m 2956
       break
~
$~
