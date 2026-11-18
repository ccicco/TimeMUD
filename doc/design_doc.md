# TimeMUD — Design Document

**Status:** Living document. Consolidates all settled design as of 2026-11-18. Sections marked `OPEN` await a decision; everything else is standing design. When this doc outgrows copy-paste, it gets patched in place.

**Scope:** A MUD written from scratch in modern C++. NOT a port. The text act of the world "Age of Chronicles"; the same simulation is later re-experienced as a graphical game. Repo: `timemud`.

---

## 1. Core Thesis

> "Getting the most enjoyment possible out of the text based system then experiencing it anew in a graphical system."

The text era is the **first act**, not a prototype. Sim depth is the shared asset; presentation is built once per medium. Every design choice is tested against one tiebreaker: **does this deepen the sim?** A number the sim can't express is a thing no renderer can ever show.

### Names
- **TimeMUD** — the text act's name (lineage: Curtiss's original 1990s time-travel MUD concept, where time travel was the mechanism to reach different eras, not the point).
- **"Age of Chronicles"** — the durable world/brand name that survives the medium change.
- Names are config strings, never baked into architecture.

---

## 2. Founding Principle

> "I view a chosen class as a chosen skillset."

The substrate is **skills** (use-based growth). Classes, occupations, backgrounds, primes, and eras are **data curations over the skill space**: bundles, aptitude discounts, gates, obligations, and story labels. Classes never grow; they shape learning. The engine thinks in skills; content and players think in classes; class count is infinite and free (naming a skillset costs lore, not code).

**The world is one giant skill pool** (possibility-space); a character is a **slice** carved by choices. Anti-goal: a player who can reach everything cheaply has no character, just an account.

**Rule:** Every choice *narrows before it opens* — commitments are self-imposed exclusions that buy depth. Changeable prime and open acquisition keep exclusions honest (costs and doors back out, not permanent walls).

**Boundary (settled):** commands/movement/basics are **capabilities** (what you can attempt), not skills. Skills modify success and unlock variants. The pool is about growth, not locomotion.

### Slice architecture (commitment order)
1. **ERA** — outermost frame; current age's pool only; older = latent / lost / researchable
2. **BODY** — race and grafts grant unchosen entries and bar others
3. **PRIME / APTITUDE FOCUS** — the big reversible commitment (conversion re-weights)
4. **OCCUPATIONS** — access packages: gates, discounts, trainers, obligations, ladders
5. **DISCOVERY** — trainers met, research, relics, ancestor chronicles
6. **PRACTICE BUDGET** — attention as the final filter

**Era dividend:** the pool is era-scoped but persistent. All eras' pools are the world's accumulated possibility-space; era-two scholars research era-one entries as lost techniques. History is a compounding library of what was learnable.

---

## 3. Skills & Growth

### The unified skill pool
Every learnable/practicable/possessable ability is one **SkillDef** in one pool: spells, languages/literacy, crafts, social skills (persuasion, deception), meta-skills (leadership, teaching — masters become trainers and the world gets schools naturally; administration). Racial/grafted abilities are pool entries **granted by body** rather than learned — one pool, uniform perception/effects/chronicle treatment.

Spells are skills with magic recipes: **Verb × Domain grammar** (project/ward/evoke/compel/transform/reveal × flame/wind/earth/mind/shadow/life/time × bolt/aura/glyph/touch/ritual) is a naming/combination scheme over pool entries, yielding ~200 spells without authoring 200, and enabling novel-combination spell research as gameplay. `OPEN` at implementation: exact grammar terms.

Skills are **data recipes, not functions**: `SkillDef{id, era, name, domain, action, cost, check, effects[], triggers, gates}` with ~15–20 effect primitives. Effects as data now = animations as data later. Balance is tuned at the primitive level; skills inherit balance. Target breadth: **50–100+ skills per era**, organized with MegaTraveller-style cascades (Firearms ← Slug Thrower ← Rifle).

### Three-value growth (validated by SneezyMUD's 30-year implementation)
- **Natural potential** — the ceiling, from prime/aptitude (TimeMUD's replacement for class/level)
- **Trained** — practice points raise the value toward the ceiling
- **Do-learnedness** — use raises the value under the ceiling

Practice buys ceiling; use buys value. The explicit split.

### Consequence-driven growth
The growth signal is the **event and its outcome**, not abstract XP. No kill-10-rats economy.
- **Skills grow from consequence-weighted use:** difficulty and stakes modulate growth; a barely-succeeded hard pick is worth more than fifty easy ones. **Failure also teaches** (at a reduced rate) — trying hard things stays optimal even when losing.
- **Attributes grow from consequences:** haul ore → strength rises; endure wounds → constitution edges up. Attributes are slow consequence-accumulated curves, not chargen constants ("the body keeps the score"). Chargen sets the starting position, not the permanent identity; era-long arcs are visible in the character sheet.
- **Negative consequences exist:** injuries reduce capability (temporary + chronicle-permanent), skills atrophy from disuse, a crushed hand re-opens latent retraining. The scars system is the chronicle layer of this and extends into mechanics.
- **Exploit mitigations, named honestly:** diminishing returns on repeated similar events; growth brackets gated by trainers/breakthroughs (self-teach to journeyman, a master opens the next bracket); difficulty weighting favors variety. Known macro risk: "cheapest consequence per growth" farming (cf. swim-in-circles exploits).

### The contest as the unit of growth
A "kill 10 rats" scenario is not an event. Events are individual **opposed checks** inside the scenario, each exercising a specific skill.
- An encounter is a **curriculum of checks**: perception (spot the ambush), dodge (bites avoided), weapon skill (swings vs the rat's defense), armor-use (absorbed hits), tactics (chosen ground). The scenario trains a cluster in the proportions of how it actually went.
- Checks are **opposed** (weapon vs dodge). A whiffed swing at a slippery target is still a sword lesson. Attempts against a defense exercise the skill even on failure. **No kill-XP exists; the contest is the unit of growth.** The same ledger serves ten rats or one dragon.
- **Approach = curriculum** (structural anti-grind, not tuning): fight trains weapon/dodge/armor/tactics; sneak trains stealth/perception/quiet movement; collapse the nest trains engineering/demolition; persuade the rat-catcher trains persuasion/appraisal/nerve.
- **Implementation:** check-level events on the stream. Progression consumes checks (each grows its skill); the chronicle consumes **aggregates** ("cleared the granary rats alone"), never 200 raw check events. The check stream is in-memory bookkeeping, periodically **committed** in batches — never every-check-to-disk.

`OPEN` (decide at SkillDef draft time): **disciplines** — a separate learnedness axis per skill group (Sneezy's model: granular, per-skill start thresholds, raising a discipline raises all member skill caps) vs folding gates directly into occupation rank (simpler). Now informed by live play experience on a three-value system.

---

## 4. Characters

### Creation
Traits **and flaws** (flaws must inconvenience to matter), appearance/background/marks/mood slots, public-vs-private knowledge split (secrets exist), deed-earned titles. **`OPEN`:** full chargen flow.

### Reputation & memory
Reputation is a web: per-faction standing plus **per-NPC personal memory** — NPCs remember prior interactions and shopkeepers behave differently (`Relationship{trust, familiarity, last_interaction, grievances}` on NPCs). Bonds between characters: party cohesion, NPC loyalty, PC rivalry.

### Chronicle, scars, and narrative flags
Persistent story state from events (WoundedCritically, BrokeOath, SlewBoss...). Old characters' history is visible in the later graphical act — the "homecoming" payoff.

### Changeable prime
Prime = the currently aptitude-focused skillset bundle. **Conversion is a re-weighting**, not a data-layout change.
- Conversion costs **story, not stats**: practice points, slow aptitude migration, formal release of the old occupation (the guild remembers; rep hit; possible oathbreaker tag).
- Old skills go **latent** — retrain cheap, chronicle remembers; "once a thief" rule; latent skills are what bloodlines echo. Old prime's perks/aptitudes swap out (no perk-collecting).
- In-world framing is content: trainer, ritual, defection quest, era-appropriate converters.
- Anti-abuse: cooldown/escalating cost. Design target: 1–3 prime changes per long character life (mage → sword-sworn after the war → wandering lorekeeper is the *intended experience*, not an edge case).

### Unification
Backgrounds (creation) / occupations (acquired) / primes (converted) / eras (remorted into heirs) = **one mechanism at four scales**.

---

## 5. Occupations

Inspiration: Rifts OCCs, made learnable. An occupation is a learnable skillset package, not a chargen straitjacket:

`OccupationDef{id, era, name, tier, entry_prereqs (stats/skills/rep/lineage/oath), teaches (skill tree + aptitude discounts), minor_perks (sideways, never power spikes), rep_ties, obligations (dues/oaths/availability/restrictions), advance_to ladders}`

- **Open acquisition:** anyone can learn anything through gates; classes are starting backgrounds only. Effective variety is combinatorial.
- **Multi-occupation budget:** active obligations are capped; formally release an occupation (with reputation consequences) to take a new one.
- **Content-first growth:** new occupations enter as specializations/hybrids tied to world fiction; each must name a trainer, a hall, and rivals, or stay lore-only (NPCs remember occupations before they're mechanical).
- **No engine-level class/occupation cap, ever.** Content quality is the only throttle. Eras multiply the space; the old era's set retires into history with no cross-era balance collision.

---

## 6. Eras

Different eras are different **development environments**, offering different PC classes with different skillsets. Classes are era-native packages — `ClassDef{era, skill_gates, stat_biases, advancement_rules}` — DATA, never hard-coded behavior. An `EraDef{classes, advancement_weights, available_skills}` weights advancement differently (mythic deed-based bronze age vs guild-structured medieval vs crafting/economy industrial). **Timeless skills** (perception, lore, persuasion, craft) survive eras; class packages don't.

### Era change: INHERITANCE (direction settled, details open)
The era ends as a world event in fiction; characters retire into world history; the next character is an **heir**.
- **Flows down fully:** reputation (faction + family fame/infamy), deed-earned titles, chronicle-as-lore, material legacy (estate, ancestral sword; estate yes, worn gear no).
- **Flows down partially:** traits (weighted, flavored as upbringing), a small capped lineage bonus in timeless skills only.
- **Does not flow:** levels, XP, class, era-bound skills.
- **Heir creation:** the player picks from a menu of inherited **edges and burdens** — burdens are attractive (family debt, the old rival's son, an unkept oath = quest generators). Player agency filters inheritance; history has teeth but no pre-play death rolls.
- `OPEN`: transition style — live world-event vs chapter-based (era two launches as the server's next chapter, old characters archived to legend). Chapter-based is flagged as more tractable and more thematic. **Awaiting call.**

### The unoccupied gap TimeMUD fills
Precedent MUDs spatialize time (LegendMUD's coexisting era regions; 4 Dimensions' time-travel dimensions): eras coexist, history never happens, characters are era-tourists. TimeMUD: **sequential eras as chapters of one continuous history; the world remembers; characters are dynasties.** Content rule stolen from LegendMUD: an era contains what its people believed — era-one myths can be wrong in ways era-two scholars discover (an inheritance dividend).

### Era one proposal (NOT yet confirmed)
Mythic/bronze age; advancement by deeds (some practice points from chronicle-worthy acts, not just XP). Five archetype classes: **Warrior** (spatial combat showcase), **Seer** (lore/perception/prophecy — information, not fireballs), **Speaker** (persuasion + faction rep + NPC memory as weapon), **Smith** (crafting, material legacy, economy), **Wanderer** (scouting, range bands, traits/flaws showcase).

---

## 7. Engine & Architecture

**C++17/20, idiomatic:** std::string, smart pointers, std::variant + std::visit for event types, `unordered_map<ID, Entity>` world index, std::chrono timing, std::optional, string_view parsing. IDs across save boundaries; no pointers persisted.

### Three layers
1. **Headless simulation core** — no I/O assumptions
2. **Presentation** — `OutputSink` interface; TextSink now, GraphicsSink/JSON-websocket later
3. **Persistence** — separate module

### Event-stream core
Every game event is a message object — the spine both renderers consume:

`Event{sequenceId, GameTime(tick + era), subject, target?, room + optional position/rangeBand, category (Combat/Movement/Speech/Social/World/System/Progression), payload (std::variant typed payloads, closed and versioned; never mutate existing members), visibility (a NUMBER not bool: full/partial-heard/none — serves stealth and graphical fog-of-war)}`

- Text client renders events as prose; graphical client renders the same events as animations. **One stream, two renderers.**
- Chronicle-bearing events carry flags in payload (ScarTag, OathEvt, deeds): chronicle/scars/rep/heir-legacy read the same stream. The event log **is** world history, replayable into lore for the graphical act.
- Presentation data stays out of events (SpokeEvt carries text because speech's payload is text; AttackEvt carries numbers, prose composed at TextSink).
- **Progression is a subscriber to the stream** (`SwingEvt{difficulty, outcome, stakes}` → renderer prints, chronicle flags deeds, progression grows). One event, three consumers.

### Networking
Single-threaded `poll()` loop, non-blocking sockets, **no asio** (unless/until TLS or websockets arrive, and then as a boundary module). EINTR-safe by design. Debt-carry timing from day one. Clean shutdown paths designed in (FredMUD lessons applied at birth).

### Persistence
Text/JSON world + character files, **schema-versioned from commit one** — never raw `fwrite(struct)` (FredMUD class-14 lesson; Sneezy's charFile struct is the cautionary exhibit). Save on change or clean shutdown. **Separate persistent world state from resettable spawn state** (reset the goblins, keep the collapsed mine). Character record carries **era + lineage fields in schema v1**.

### Build system (settled)
**Plain Makefile**, not CMake. One file, no meta-build step; `make` with Termux clang, desktop Linux gcc unchanged. Separate `.o` per source, **header deps tracked** (unlike FredMUD's Makefile gap), `make clean` from day one, `-Wall -Wextra` on, standard flag set once. Portability rule: **no OS-specific headers in core files**; any platform code isolated in a `net/platform` module; POSIX APIs only — Termux clang and desktop gcc build the same tree with zero ifdefs if possible.

### Spatial model
- **Rooms:** stateful — conditions (darkness, fire, flooded, bloodied), wear, event memory (battlefield descriptions persist). Descriptions are **assembled** from base + condition flags + event traces; flags render as text now, visuals later.
- **Exits:** stateful entities (locked, broken, hidden, one-way, conditional). Player-built structures possible later (rooms as runtime data).
- **Every room gets size + (x,y,zone) coordinates in schema from day one** — retrofit is miserable; needed for wilderness gen, mapping, graphics.
- **Movement & combat:** v1 = **range bands inside rooms** (melee/near/far/edge; advance/retreat/close; ranged weapons need range). Upgrade path to internal coordinates later without save-breaking migration; full coordinate world deferred until earned. Combat becomes spatial (reach, LOS, ranged arcs, formations); AI pathing gets a cost budget; the event schema carries optional position/range.
- Parked for later: wilderness as deterministically generated space from seed(x,y) with set-piece overrides.

### Build order (agreed direction)
1. Socket accept + line input/echo
2. Login state machine
3. Rooms + movement + look
4. Save/load
5. Tick loop
6. Combat
7. Events-based output refactor **early** — before much content logic, or it gets expensive

**Round 0 (awaiting GO):** project skeleton — repo, Makefile, three-layer directory split, first compile of an empty core.

---

## 8. Body: race & grafts

Rule born from TempusMUD's buggy cyborg class: **RACE IS BODY** (stat biases, natural traits, world perception), **OCCUPATION IS SKILLSET** — never mixed. Grafts are stateful body modifications: era-bound tech, persistent, chronicle-bearing, same data family as scars; chronicle-worthy horror in early eras, normal trade later. Feeds NPC perception and graphics.

---

## 9. Reference Card (what we learned from others)

| Source | Lesson taken |
|---|---|
| SneezyMUD (C++, 1992–now) | Three-value growth proven at 500+ abilities; trainers as world content; quest-toggle unlocking; one enum + one info struct scales; practice buys ceiling, use buys value |
| LegendMUD (1994–now) | Few, deep eras sustain decades; era contains what its people believed; myths can be wrong |
| 4 Dimensions (1996) | In-fiction agencies beat mechanical portals; era texture appeals; hub-and-spoke commons |
| TempusMUD (played) | The cyborg-class bug → race/body vs occupation/skillset separation |
| SlothMUD | Remort past caps; locked prime (our prime is changeable by design); live-operation class growth minus recompiles |
| Traveller / MegaTraveller | Prior-history characters; inheritance with player agency; skill cascades |
| Rifts | Occupations as way-of-life packages |
| FredMUD (own) | Header deps in Makefile; clean shutdown; debt-carry timing; no raw struct writes |

---

## 10. Open Decisions Ledger

| # | Question | Status |
|---|---|---|
| 1 | Era transition: live world-event vs chapter-based | Awaiting call (chapter-based recommended) |
| 2 | Era one: mythic/bronze age, 5 archetype classes | Proposed, not confirmed |
| 3 | Disciplines as separate axis vs occupation-rank gating | Decide at SkillDef draft; now informed by live Sneezy play |
| 4 | Spell grammar exact terms (Verbs × Domains × Forms) | OPEN at implementation |
| 5 | Full chargen flow | OPEN |
| 6 | Era transition fiction (live-event vs chapter) mechanics | Follows decision 1 |

**Ritual:** rounds are GO-gated; compile gate is Curtiss's Termux build; first-try landing discipline; handoffs are copy-paste; round state persists to knowledge immediately when a gate passes.
