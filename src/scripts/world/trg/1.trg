#100
Anella Welcome Speech~
0 g 100
~
%send% %actor% Anella  stops wiping the counter as she sees you and flashes you a smile.
%send% %actor%
if (%actor.sex% == MALE)
  %send% %actor% Hi there sweetie, just take a seat at the bar, or one of the tables and
else if (%actor.sex% == NEUTRAL)
  %send% %actor% Err... hello, just take a seat at the bar, or one of the tables and
else
  %send% %actor% Hi there miss, just take a seat at the bar or one of the tables and
end
%send% %actor% I'll get to you as soon as possible.  If you're hungry or thirsty just ask
%send% %actor% and I'll bring you a @Wmenu@n.  Oh and Otik wants to apologize if the service
%send% %actor% is slow.  There's just so much @Wwork@n to be done these days, especially since
%send% %actor% Tika disappeared that night.  Anyway let me know if you need something, ok?
%send% %actor%
%send% %actor% Anella  smiles and then turns away to start tending to some other customers.
~
#101
Caramon Welcome Speech~
0 g 100
~
%send% %actor% 
%send% %actor% Caramon smiles as you approach, and waves one of his huge hands in
%send% %actor% your direction.  He says, 'Hi there, you look like the adventuring sort.
%send% %actor% Solace has been having some @Wproblems@n of late and we could really use
%send% %actor% some help.  I'd do it myself, but well... whenever I suggest anything Tika
%send% %actor% reminds me how the War is over and we've done our part and... oh well..
%send% %actor% Hey, but listen, if you're intertested just let me know and we'll figure
%send% %actor% out how you can help and how you'll be compensated.'  He finishes with
%send% %actor% a very rough clap on the back, and then turns his attention to other things.
%send% %actor% 
~
#102
Caramon Speech Solace's Problems~
0 d 0
problems~
%send% %actor%
%send% %actor% Well there's a lot of things going on right now but I think the biggest
%send% %actor% thing is that, well, ever since the war ended, and Tika and I took over
%send% %actor% this place, adventurers have been coming here from all over.  Solace has
%send% %actor% become a bit of an adventuring town ever since.  Anyway a while back, after
%send% %actor% talking with Tanis, we decided it would be best to set up some kind of training
%send% %actor% facility, so at least people wouldn't go out and get themselves slain by the
%send% %actor% first draconian they came across.  Anyways, we set up some training pits just
%send% %actor% south of the fountain, and things went well for while.  But lately, the guy
%send% %actor% who runs the place has been barely ever seen here in town, and when he was
%send% %actor% he was acting very, very strange.  Soon after adventurers stopped coming out.
%send% %actor% I'm obviously assuming the worst.  Do me a favor and check things out down
%send% %actor% there?  It's dangerous, I won't lie to you, but you look capable, and it
%send% %actor% pays 10 steel coins if you come back with some information.  You can make that
%send% %actor% 100 more if you can solve the problem entirely.  Come back and tell me the
%send% %actor% pits have been restored when you have something concrete to show.
~
#103
Caramon Speech The Pits Have Been Restored~
0 d 0
the pits have been restored~
%send% %actor%
%send% %actor% Caramon exclaims, 'Really?  What have you learned?  Give me any evidence you have'
%send% %actor% found!'
%send% %actor%
~
#104
Caramon Evidence of Renthilos's Plot~
0 j 100
~
if (%obj.vnum% == 902)
  %send% %actor% @WWhat's this?  Evidence of a plot by Renthilos?  Let me take a quick peek...
  %send% %actor% Hmm seems complicated to me.. but yeah he's definitely up to no good.  You
  %send% %actor% have my authority to stop this plot by whatever means necessary.  Bring
  %send% %actor% back proof when the deed is done.  Good work so far though!  Here's your
  %send% %actor% 10 steel coins just as I promised.  Remember, it's 100 more if you thwart his plans.@n
%load% obj 106
give purse %actor.name%
end
~
#105
Priestess Shannara Healing Script~
0 d 0
neutralize poison~
sa @W@WHmm let me take a look at you...@n@n
wait 1
examine %actor.name%
wait 1
if (%actor.affect(poison)%)
sa @W@WAhh, now how did this happen... you seem to be poisoned my dear.  Let me help you.@n@n
wait 1
sa @W@WOh, blessed Mother, Healing Hand of all those in need of her comfort...@n@n
sa @W@WPlease bless this poor soul, and remove the debilitative toxin from them...@n@n
sa @W@WThat they may continue in peace, and find thee and the light ever after, Amen@n@n
dg_cast 'neutralize poison' %actor%
wait 1
sa @W@WThere, all better.  Please try to be more careful next time.@n
smile
else
sa @W@W My dear, there's nothing wrong with you whatsoever.  Perhaps you simply need some rest.@n@n
smile %actor.name%
end
~
$~
