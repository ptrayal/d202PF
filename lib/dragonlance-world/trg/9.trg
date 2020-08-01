#900
Listen Check - Overhear Renthilus' Plans~
2 g 100
~
if (%random.20% + %actor.skill(listen)% > 18)
%send% %actor% @n
  %send% %actor% @WYou hear a harsh voice on the other side of the door speaking angrily,
  %send% %actor% 'What do you mean you lost it?  Well listen to me closely you pathetic worm!
  %send% %actor% Either find it now, or spend the rest of your brief life being pulled apart
  %send% %actor% by the limbs by some of your own creations! Now go!  If you said it was
  %send% %actor% near the southeast corner, I suggest you start your search there!@n'
%send% %actor% @n
end
~
#901
Sheaf of Incriminating Papers - Intelligence Check for More Info~
1 c 2
think~
if ("%arg%" == "papers")
if (%random.20% + ((%actor.int% - 10) / 2) > 15)
%send% %actor% @W@WIt seems as if Renthilos has underestimated the adventuring community.
  %send% %actor%  It turns out he has a knack for flocking morons to his banner, but not
  %send% %actor% for planning, problem solving, or generally making even a plan as detailed
  %send% %actor% as this come to fruition.  Judging by the footnotes made by the apprentices
  %send% %actor% a handful of them would be able to overthrow him if they could get their act
  %send% %actor% together themselves.  For professionals like yourself, it should be a piece
  %send% %actor% of cake@n@n.
end
end
if ("%arg%" == "paper")
if (%random.20% + ((%actor.int% - 10) / 2) > 15)
%send% %actor% @WIt seems as if Renthilos has underestimated the adventuring community.
%send% %actor%  It turns out he has a knack for flocking morons to his banner, but not
%send% %actor% for planning, problem solving, or generally making even a plan as detailed
%send% %actor% as this come to fruition.  Judging by the footnotes made by the apprentices
%send% %actor% a handful of them would be able to overthrow him if they could get their act
%send% %actor% together themselves.  For professionals like yourself, it should be a piece
%send% %actor% of cake@n.
end
end
if ("%arg%" == "sheaf")
if (%random.20% + ((%actor.int% - 10) / 2) > 15)
%send% %actor% @WIt seems as if Renthilos has underestimated the adventuring community.
%send% %actor%  It turns out he has a knack for flocking morons to his banner, but not
%send% %actor% for planning, problem solving, or generally making even a plan as detailed
%send% %actor% as this come to fruition.  Judging by the footnotes made by the apprentices
%send% %actor% a handful of them would be able to overthrow him if they could get their act
%send% %actor% together themselves.  For professionals like yourself, it should be a piece
%send% %actor% of cake@n.
end
end
~
$~
