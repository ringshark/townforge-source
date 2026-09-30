# Meshy asset workflow

The existing hero, eight outfit variants and weapon assets already originated in
Meshy. See `tools/characters3d/README.md` for the animation merge, retargeting,
dye regions and grip normalization used by this game. Reuse these before
generating replacements.

## Connect once

1. In Meshy, open Developer Platform → API Keys. A paid plan is required.
2. Create a key named Town Forge, with a monthly credit cap (start with 50).
3. In ringshark/townforge-source → Settings → Secrets and variables → Actions,
   create a repository secret named `MESHY_API_KEY`, with that key as its value.
4. Run Actions → Meshy Assets on branch `ringshark-patch-1`, operation `check`.
   This checks authentication with a read-only task listing. It submits no
   generation and does not log the key or your asset history.

The workflow is registered on the default branch so GitHub displays the Run
workflow button. It always checks out `ringshark-patch-1` for its tooling; the
active game source branch stays unchanged.

## Download and generation

Use `download` with an existing API task ID and its task type to stage its GLB
outputs. Rigging tasks include their walking/running outputs when available.
The API's task history is separate from web-app My Assets; an old model created
in the Meshy web app may need to be exported manually instead.

Use `preview` with a prompt to submit exactly one Meshy 6 untextured model.
Use `refine` with its successful preview ID to texture that model separately.
Both operations spend credits; check current API pricing and the key's cap.
An API timeout never automatically retries task creation. If a task ID was
returned, it is saved in the run artifact: use `download` to resume it.
If no ID was returned, check Meshy's request logs before resubmitting.

Downloads are in the run's `meshy-assets-*` artifact, retained for 30 days.
Files are checked for a complete GLB 2.0 header and staged for review. Signed
download URLs and credentials are excluded from the saved manifest.
This workflow does not overwrite game assets, commit exports or deploy them.
Downloaded characters still require skeleton/animation, grip, texture and
equipment-fit checks using the existing conversion scripts before integration.

Reference: https://docs.meshy.ai/en/api/authentication and
https://docs.meshy.ai/en/api/text-to-3d .
