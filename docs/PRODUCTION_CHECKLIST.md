# Production checklist

Before publishing to Geode Index:

1. Replace PrototypeBackend with authenticated server state.
2. Never authorize owner actions only by a client-supplied account ID.
3. Make reward claims idempotent (same completion cannot pay twice).
4. Enforce name/sticker privacy server-side.
5. Rate-limit profile, search, claim, purchase and rate endpoints.
6. Validate display-name length and control characters server-side.
7. Keep sticker purchases disabled until the actual 2.209 assets/API are known.
8. Test conflicts with mods that alter GJAccountSettingsLayer, ProfilePage, CommentCell or LevelInfoLayer.
9. Test Android touch targets and keyboard layout.
10. Verify CANCEL / UPDATE semantics after every UI change.
