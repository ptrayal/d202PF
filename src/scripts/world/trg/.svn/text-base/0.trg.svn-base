#0
undefined~
0 gh 100
~
* No Script
~
#1
Escape from Limbo~
2 c 100
~
* assign this to a mob, force the mob to mremember you, then enter the
* room the mob is in while visible (not via goto)
say I remember you, %actor.name%!
~
#2
mob greet test~
0 g 100
~
if %direction%
  say Hello, %actor.name%, how are things to the %direction%?
else
* if the character popped in (word of recall, etc) this will be hit
  say Where did YOU come from, %actor.name%?
end
~
#3
obj get test~
1 g 100
~
%echo% You hear, 'Please put me down, %actor.name%'
~
#4
room test~
2 g 100
~
wait 50
wsend %actor% you enter a room
~
#5
car/cdr test~
0 d 100
test~
say speech: %speech%
say car: %speech.car%
say cdr: %speech.cdr%
~
#6
subfield test~
0 c 100
test~
* test to make sure %actor.skill(skillname)% works
say your hide ability is %actor.skill(hide)% percent.
*
* make sure %actor.eq(name)% works too
eval headgear %actor.eq(head)%
if %headgear%
  say You have some sort of helmet on
else
  say Where's your headgear?
  halt
end
say Fix your %headgear.name%
~
#7
object otransform test~
1 jl 7
test~
* test of object transformation (and remove trigger)
* test is designed for objects 3020 and 3021
* assign the trigger then wear/remove the item
* repeatedly.
%echo% Beginning object transform.
if %self.vnum% == 3020
  otransform 3021
else
  otransform 3020
end
%echo% Transform complete.
~
#8
makeuid and remote testing~
2 c 100
test~
* makeuid test ---- assuming your MOBOBJ_ID_BASE is 200000,
* this will display the names of the first 10 mobs loaded on your MUD,
* if they are still around.
eval counter 0
while (%counter% < 10)
  makeuid mob 200000+%counter%
  %echo% #%counter%      %mob.id%   %mob.name%
  eval counter %counter% + 1
done
%echoaround% %actor% %actor.name% cannot see this line.
*
*
* this will also serve as a test of getting a remote mob's globals.
* we know that puff, when initially loaded, is id 200000. We'll use remote
* to give her a global, then %mob.globalname% to read it.
makeuid mob 200000
eval globalname 12345
remote globalname %mob.id%
%echo% %mob.name%'s "globalname" value is %mob.globalname%
~
#9
mtransform test~
0 g 100
~
* mtransform test
* as a greet trigger, entering the room will cause
* the mob this is attached to, to toggle between mob 1 and 99.
%echo% Beginning transform.
if %self.vnum%==1
  mtransform -99
else
  mtransform -1
end
%echo% Transform complete.
~
#10
attach test~
0 d 100
attach~
attach 9 %self.id%
~
#11
detach test~
0 d 100
detach~
detach 9 %self.id%
~
#12
spellcasting test~
0 c 100
kill~
* This command trigger will disallow anyone from trying to
* use the kill command, and will toss a magic missile at them
* for trying.
dg_cast 'magic missile' %actor%
return 0
~
#20
room global random trigger test~
2 ab 100
*~
* This trigger will fire every PULSE_DG_SCRIPT - standard os 13 secs.
%echo% 13 Seconds have passed!
~
#21
room random trigger test~
2 b 100
~
* This trigger will fire every PULSE_DG_SCRIPT - standard os 13 secs.
* If someone is here to see it.
%echo% 13 Seconds have passed!
~
#22
room command trigger test~
2 c 100
inv~
* This handy little test command will show everybody your inventory. 
eval first_item %actor.inventory%
%echo% %actor.name% is using:
while %first_item%
  %echo% %first_item.name%
  eval first_item %first_item.next_in_list%
done
eval weapon %actor.eq(wield)%
if %weapon%
  %echo% And %actor.heshe% is using %weapon.name% as a weapon.
end
~
#23
room speech trigger test~
2 d 100
*~
* Anything you say, can and will be used against you.
* So it will run for anyone...
context %actor.id%
%send% %actor% The room starts echoing your words:
while %random.10% > 3
wait 1s
%echo% The walls reflect the words: ..%speech.cdr%..
done
~
#24
room zone reset trigger test~
2 f 100
~
* Will open a door, then remove it totally every zone reset.
%door% 0 north room 3000
wait 30 s
%door% 0 north purge 0
~
#25
room enter trigger test~
2 g 100
~
if (%actor.level% > 10)
  %send% %actor.name% You're too experienced to enter here.
  return 0
end
~
#26
room drop trigger test~
2 h 100
~
if (%object.type% == TRASH)
  %echo% No Littering!
  return 0
end
~
#27
room cast trigger test~
2 p 100
~
if '%spellname%'=='magic missile'
  %send% %actor% Your puny magic will not work inhere!
  return 0
  halt
end
return 1
~
#99
undefined~
0 gh 100
~
* No Script
~
$~
