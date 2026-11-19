# Test inputs

Each file holds the keystrokes for one test case. Run from the project folder (the one with `src/` and `data/`).
Set `DBND_SEED` so the random enemies are the same every run.

Windows (Command Prompt):

```
set DBND_SEED=2
battle_sim.exe < tests\t06_cooldown.txt
```

| File | What it checks |
| --- | --- |
| t01_normal.txt | New single-player Mage game, one attack |
| t02_recharge_max.txt | Recharge at 6 MP gains 0 MP |
| t03_invalid_menu.txt | `abc`, `9`, empty line and `-1` at the main menu are rejected without a crash |
| t04_save_part1.txt, t04_save_part2.txt | Clear Stage 1 (use `DBND_SEED=7`) then run part 2: Continue reloads Stage 2 |
| t05_duplicate_name.txt | `lee` after `Lee` is rejected |
| t06_cooldown.txt | Thrust (cooldown 3) shows `Cooldown: 2 turns` on turn 2 and is refused |
| t07_not_enough_mp.txt | A 2 MP move with 1 MP shows the not-enough-MP message |

