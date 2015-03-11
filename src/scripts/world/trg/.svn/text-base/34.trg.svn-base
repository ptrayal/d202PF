#3400
Customizable Prompt~
2 c 2
*~
if %cmd.mudcommand% == score
  return 0
elseif %cmd% == help && %arg% == prompt
%send% %actor% Syntax:
%send% %actor% fprompt <commands>
%send% %actor% fprompt remove
%send% %actor% fprompt default
%send% %actor% prompt default
%send% %actor% prompt <commands>
%send% %actor% prompt remove
%send% %actor% prompt2 <commands>
%send% %actor% prompt2 remove
%send% %actor% prompt3 <commands>
%send% %actor% prompt3 remove
%send% %actor% tcolor
%send% %actor% display
%send% %actor% display default
%send% %actor% @n
%send% %actor% @RImmortal ONLY!:@n
%send% %actor% @n
%send% %actor% display default <name>
%send% %actor% display <name>
%send% %actor% @n
%send% %actor% Prompt default will set your prompt to a default setting, with only the
%send% %actor% bare minimum of information posted in your prompt. As well, it will
%send% %actor% remove prompt2 and prompt3 if you are using either extended prompts.
%send% %actor% @n
%send% %actor% Prompt remove, prompt2 remove, and prompt 3 remove all remove their
%send% %actor% respective lines of the prompts but leave the others untouched. Keep in
%send% %actor% mind, if you have a prompt2 and prompt3, and you remove prompt2, it will
%send% %actor% APPEAR that you have your prompt3 set to prompt2. This is not the case,
%send% %actor% your second line will still be remembered as prompt3.
%send% %actor% @n
%send% %actor% Tcolor stands for "toggle color". This will automate percentile coloring
%send% %actor% for life/mana/move/ and opponent's hp in your fprompt..
%send% %actor% @n
%send% %actor% Typing prompt with commands following it, will display anything you
%send% %actor% enter in as a command. Prompt2 will do the same, but seperate it with
%send% %actor% a line break, making it a second line of information. Prompt3 will do
%send% %actor% this yet again, providing up to 3 lines of information you can cram
%send% %actor% pack with the following commands plus ANYTHING you type.
%send% %actor% @n
%send% %actor% Valid Fields for the prompt are as follows:
%send% %actor% @n
%send% %actor% Prompt Commands are:
%send% %actor% %%h%%  - current hp
%send% %actor% %%mh%% - max hp
%send% %actor% %%m%%  - current mana
%send% %actor% %%mm%% - max mana
%send% %actor% %%v%%  - current move
%send% %actor% %%mv%% - max move
%send% %actor% %%g%%  - current gold
%send% %actor% %%e%%  - current experience
%send% %actor% %%s%%  - room sector
%send% %actor% %%exits%% - displays exits in current room
%send% %actor% %%send%% %actor% @n
%send% %actor% %%n%%  - GOD ONLY displays room number
%send% %actor% @n
%send% %actor% %%\%%  - used for displaying a common / symbol (was required)
elseif %cmd% == tcolor
  if !%actor.varexists(tcolor)%
    set tcolor 1
    remote tcolor %actor.id%
    %send% %actor% Tcolor is now on.
  else
    rdelete tcolor %actor.id%
    %send% %actor% Tcolor signing off.
  end
elseif %cmd% == prompt || %cmd% == prompt2 || %cmd% == prompt3
  if !%arg%
    %send% %actor% What would you like your prompt to consist of?
    %send% %actor% Type Help Prompt for more information.
    halt
  end
  if %arg% == default
    rdelete prompt %actor.id%
    rdelete prompt2 %actor.id%
    rdelete prompt3 %actor.id%
    halt
  elseif %arg% == remove
    rdelete %cmd% %actor.id%
    halt
  elseif %arg.contains(actor)% || %arg.contains(self)%
    halt
  end
  set %cmd% %arg%
  remote %cmd% %actor.id%
else
  if %actor.level% > 30
    set v %actor.room.vnum%
  end 
  if %actor.varexists(tcolor)%
    set h %actor.hitp%
    set m %actor.mana%
    set v %actor.move%
    set oh %actor.fighting.hitp%
  else
    set 1 %h% %actor.hitp% %actor.maxhitp%
    set 2 %m% %actor.mana% %actor.maxmana%
    set 3 %v% %actor.move% %actor.maxmove%
    set 4 %oh% %actor.fighting.hitp% %actor.fighting.maxhitp%
    set i 1
    while %i% < 5
      extract var 1 %i%
      extract cur 2 %i%
      extract max 3 %i%
      set p ((%cur% * 100) / %max%)
      if %p% > 90
        set %var% @W%cur%
      elseif %p% > 70
        set %var% @G%cur%
      elseif %p% > 40
        set %var% @Y%cur%
      elseif %p% > 10
        set %var% @R%cur%
      else
        set %var% @D%cur%
      end
      eval i %i% + 1
    done
  end
  set mh %actor.maxhitp%
  set mm %actor.maxmana%
  set s %actor.room.sector%
  set mv %actor.maxmove%
  set g %actor.gold%
  set e %actor.exp%
  set exits Exits:
  set exit north east south west up down
  set onelet n e s w u d
  set i 1
  while %i% < 7
    extract dir %i% %exit%
    extract d %i% %onelet%
    if %actor.room.%dir%%
      if %actor.room.%dir%(bits)% /= CLOSED
        set exits %exits% (%d%)
      else
        set exits %exits% %d%
      end
    end
    eval i %i% + 1
  done
  wait 1
  if %cmd% == display
    return 1
    if !%arg%
      set string prompt prompt2 prompt3
      set i 1
      while %i% < 4
        extract prmt %i% %string%
        if %actor.varexists(%prmt%)%
          eval prompt %actor.prompt%
          %send% %actor% %prompt%@n
        end
        eval i %i% + 1
      done
      if !%actor.varexists(prompt)% && !%actor.varexists(prompt2)% && !%actor.varexists(prompt3)%
        %send% %actor% @w[@cHP@w: @W%h% @w][@cMana@w: @W%m% @w](@cMove@w: @W%v%@w )@C>@n
      end
    elseif %arg.car% == default
      if %arg.cdr%
        eval tar %arg.cdr%
        if %tar.vnum% == -1 && %actor.level% > 31
          %send% %actor% @w[@cHP@w: @W%tar.hitp% @w][@cMana@w: @W%tar.mana% @w](@cMove@w: @W%tar.move%@w )@C>@n
          halt
        end
      else
        %send% %actor% @w[@cHP@w: @W%h% @w][@cMana@w: @W%m% @w](@cMove@w: @W%v%@w )@C>@n
      end
    elseif %actor.level% > 31 && %arg%
      if !%arg.cdr%
        if %arg.vnum% == -1
          set string prompt prompt2 prompt3
          set i 1
          while %i% < 4
            extract prmt %i% %string%
            if %actor.varexists(%prmt%)%
              eval prompt %actor.prompt%
              %send% %actor% %prompt%@n
            end
            eval i %i% + 1
          done
        else
          %send% %actor% That is not the name of a player.
        end
      else
        %send% %actor% You have typed too many arguments, please type: display <player's name>
      end
    end
  elseif %cmd% == fprompt
    return 1
    if !%arg%
      if !%actor.varexists(fprompt)%
        %send% %actor% @w[@cHP@w: @W%h% @w][@cMana@w: @W%m% @w](@cMove@w: @W%v%@w )(@cOpponent's Health@w: @W%oh%%%@C>@n
      else
        eval fprompt %actor.fprompt%
        %send% %actor% %fprompt%@n
      end
    else
      if %arg.contains(actor)% || %arg.contains(self)%
        halt
      end
      if %arg% == default || %arg% == remove
        if %actor.varexists(fprompt)%
          rdelete fprompt %actor.id%
        else
          halt
        end
      end     
      set fprompt %arg%
      remote fprompt %actor.id%
    end
  end
  set string prompt prompt2 prompt3
  set i 1
  while %i% < 4
    extract prmt %i% %string%
    if %actor.varexists(%prmt%)%
      eval prompt %actor.prompt%
      %send% %actor% %prompt%@n
    end
    eval i %i% + 1
  done
  if !%actor.varexists(prompt)% && !%actor.varexists(prompt2)% && !%actor.varexists(prompt3)%
    %send% %actor% @w[@cHP@w: @W%h% @w][@cMana@w: @W%m% @w](@cMove@w: @W%v%@w )@C>@n
  end
  if !%actor.varexists(prompt)% && !%actor.varexists(prompt2)% && !%actor.varexists(prompt3)%
    %send% %actor% @w[@cHP@w: @W%h% @w][@cMana@w: @W%m% @w](@cMove@w: @W%v%@w )@C>@n
  end
end 
~
$~
