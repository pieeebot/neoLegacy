# Scope of Project
At the moment, this project's scope is focused on adding gameplay content to the game (blocks, mobs, items, mechanics) following the original release order of Minecraft: Legacy Console Edition Title Updates, **starting from TU19** (Xbox 360 Edition, December 18, 2014) and moving forward from there.

All content from Title Updates prior to TU25 is considered already present in the base game. We do not reimplement or revisit pre-TU25 content unless it is buggy or broken.

## Parity
We are implementing LCE content as it existed at the time of each Title Update, in the order they were originally released. This means we will not be accepting changes that...
- Add blocks, items, mobs, or mechanics from a Title Update that has not yet been reached in our current milestone order
  - For example, submitting content from TU27 while we are still targeting TU25
- Backport things from Java Edition or Bedrock Edition that did not exist in LCE at the relevant milestone
- Add placeholder or stub implementations of content that are non-functional or misleading
  - For example, a mob with no AI, drops, or behavior, or a block with no crafting recipe or broken interactions
- Implement content in a way that is visually or behaviorally inconsistent with how it functioned in the original LCE build at that milestone

However, we would accept changes that...
- Faithfully implement content from the current target milestone in a complete, correct, and stable state
- Fix legitimately buggy or inconsistent behavior in already-implemented content that causes unexpected outcomes
  - For example, mobs clipping outside of walls, broken crafting recipes, or mechanics behaving inconsistently with the original
- Adding minimal, non-invasive Quality of Life features that support playability without straying from the core LCE experience
- Improve the quality of assets (such as sounds) while preserving their contents
  - For example, upgrading the quality of all music in-game while preserving any unique cuts / versions, or faithfully remaking those unique cuts / versions with higher quality assets
- Improve platform support, stability, or performance in ways that are transparent to the player and preserve the original look and feel

## Current Goals
- Implementing all title updates LCE had.

## Definition of "Complete" Content
A feature is not ready to merge until it meets a reasonable standard of completeness. This includes, at minimum:
- Correct world generation presence or spawn behavior (where applicable)
- Proper crafting recipe(s) or acquisition method(s)
- Correct inventory and tooltip representation
- Correct drops on destruction or death
- Functional interactions with other content present at the same milestone
- No crashes, soft-locks, or game-breaking bugs introduced by the feature

If a feature cannot be implemented fully yet, it should not be implemented at all until it can be.

# Scope of PRs
All Pull Requests should fully document the changes they include in their file changes. They should also be limited to one general topic and not touch all over the codebase unless its justifiable.

For example, we would not accept a PR that implements new blocks alongside unrelated mob AI fixes and an audio system rework, as its too difficult to review a ton of code changes that are all irrelevant from each other. However, a PR that implements a new block along with its crafting recipes, world rendering, and inventory icons would be accepted as it is one cohesive topic.

If your PR includes any undocumented changes it will be closed.

# Use of AI and LLMs
We currently do not accept any new code into the project that was written largely, entirely, or even noticeably by an LLM. All contributions should be made by humans that understand the codebase.

# Pull Request Template
We request that all PRs made for this repo use our PR template to the fullest extent possible. Completely wiping it out to write minimal information will likely get your PR closed.
