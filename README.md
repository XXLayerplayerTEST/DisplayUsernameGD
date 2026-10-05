# Display Username — 1.0 release candidate

This source build implements the final 1.0 checklist for Geode 5.10.1 / GD 2.2081.

## Included
- Large DISPLAY button in vanilla account settings; vanilla settings are otherwise untouched.
- Display Username with real GD username permanently below it in profiles; no hover/tap swap.
- Profile name is rendered on the stable ProfilePage layer instead of repeatedly fighting the vanilla username layout.
- Layer-amond balance stays in the mod menu; no custom statistic counters are injected into profiles.
- Owner-only diamond rate panel (Account ID 40224020): DU Difficulty, DU Feature, Reward, Daily, Daily Bonus, RATE IT, REMOVE DAILY, REMOVE RATE.
- Level page shows DU OPINION as the game's own difficulty icon on the opposite side from PLAY, plus the game's own Featured/Epic/Legendary/Mythic badge, reward, Daily Bonus and CLAIMED state.
- Completing a rated level claims the base reward once; Daily bonus is tracked separately per Daily generation.
- SEARCH opens the normal Geometry Dash LevelBrowserLayer using the rated level IDs; LevelCell gets a small DU reward/feature overlay.
- DAILY goes to the actual level page.
- Stickers and sticker shop items remain grey/locked until 2.209 support.
- Name Expansion Key applies 15 -> 25 immediately.

## Important prototype limitation
Layer-amond balances, rated-level data and claim state are still stored locally for testing. A public multi-user release needs an authenticated server.

## Build
From the project folder containing `CMakeLists.txt`:

```bat
geode build
```

Do not call this a final release until the Windows Geode build succeeds and the UI is tested in-game.


## 1.0 dev notes
- DU Creator Points/profile DU statistics are removed.
- Display Username can be globally disabled from the mod popup; when disabled, everyone sees the real GD username.
- Removing a diamond rate revokes any claimed base reward and the claimed Daily bonus for that active Daily rate. Layer-amonds may go negative if already spent.
- Sticker controls remain locked for 2.209: gray buttons with a centered lock.


## Display-name moderation

The text field now uses a strict character whitelist. Latin letters, numbers, spaces and the reviewed symbols `_ $ - . ! ? '` are allowed. Cyrillic is not accepted. Emoji, zero-width characters, decorative Unicode and other unusual symbols are removed immediately when typed or pasted.

A local blocked-word list refuses common profanity, explicit terms, hate/racial slurs, extremist terms and common separator/leetspeak bypasses such as `b.i.t.c.h`, `sh1t`. Refusal does not automatically ban the user.

This is only a first line of defense. A public release must repeat moderation on an authenticated server because a modified client can bypass local checks.

The Display Username settings popup was also compacted for desktop/mobile visibility.

## Owner blocked-word list (dev)
The owner account can open MOD in Display Username settings and add/remove custom blocked words (max 25 characters). The current dev build stores this list locally and uses it for local validation. A public release still needs the same list stored on the backend so all players receive the updated moderation list and suspicious-name review requests can reach the owner.


Owner profile moderation hotfix:
- Owner Account ID 40224020 sees BAN/UNBAN and DISPLAY buttons on foreign profiles.
- BAN supports seconds, minutes, hours, days, 30-day months, and permanent in the local prototype.
- DISPLAY can override/reset a remote display name locally for testing.
- Public cross-user enforcement still requires the authenticated server described in docs.
