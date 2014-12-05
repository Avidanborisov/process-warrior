process-warrior
===============

A highly effective Linux "Process War" combatant.

What is a Process War?
----------------------
Similarly to the old [Core War](http://en.wikipedia.org/wiki/Core_War), a "Process War" is a programming game for two or more battle process. Each process's objective is to kill all the enemy processes. The winner is the last running process.

The game's environment is any Linux distribution. The rules are simple:
* It's illegal to hang, crash or shut down the system.
* All process must start their logic **only** after `/tmp/GO` is created. If the program is written in C, the first line of the `main()` function has to be `while (access("/tmp/GO", F_OK)) { }`.
* All processes will be put into `/tmp` before starting the game, and will be executed from there.
Besides that, all processes can do **absolutely anything**, and can written in **any** language.
