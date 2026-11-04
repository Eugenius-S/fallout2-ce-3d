# Task 185 partial evidence

The scanner counts unregistered opcode, metarule and hook references in
available scripts. It found 2961 files and the findings in
`scan-unimplemented.txt`. Counts are static bytecode occurrences, not runtime
fires. `read_byte` in `gl_partycontrol.int` is implemented and is not listed
as a gap.

The isolated background game used the local RPU copy and a worktree-local
scene cache. Two route attempts stopped on the watchdog at `SLVCAMP.MAP`
(steps 14 and 17; 30 s and 90 s without progress). The first reported that
`floor.bin` was missing or unreadable. Neither attempt confirms dynamic
invocation counts or whether the listed gaps block play. No more runtime
attempts were made after the two consecutive failures.

The intended RP restored-area scope follows the upstream [RPU new content
list](https://github.com/BGforgeNet/Fallout2_Restoration_Project/blob/master/docs/rp-new_content.txt).
This evidence does not claim that the requested Arroyo, Klamath, Den and
restored-area tour completed. The acceptance requirement to classify dynamic
impact and create dependent task 186 remains open.

The code built with the project MSVC configuration and CTest passed 20/20.
