# Display Username

Display Username adds a separate visual username while keeping the real Geometry Dash username visible underneath on supported profile UI.

## Features

- Custom Display Username with a 15-character base limit.
- Name Expansion Key raises the limit to 25 characters.
- Real GD username remains visible below the Display Username.
- Display Username can be disabled from the mod menu.
- Layer-amond currency UI and rated-level prototype features.
- Daily/Search prototype tools and reward claim states.
- Owner moderation UI for blocked words, local bans, and local display overrides.
- Sticker-related controls remain locked until Geometry Dash 2.209 support.

## Moderation

Display names use a strict character whitelist. Cyrillic, emoji, zero-width characters, decorative Unicode, and unsupported symbols are rejected. A local blocked-word system also rejects configured terms and common bypass forms.

## Current 1.0 limitation

This build still stores Layer-amond balances, rated-level data, claims, moderation overrides, blocked-word changes, and bans locally. Cross-user synchronization and tamper-resistant enforcement require an authenticated backend and are not provided by this build.
