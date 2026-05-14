# Bug Report — CUBES Simulation

## CRITICAL

### C1. ~~`std::terminate` crash on second training session~~ **FIXED**
**Fix:** `run_training()` now joins the previous thread before reassigning (`src/main.cpp:206-207`).

### C2. ~~Same-index food double-erase corrupts food list~~ **FIXED**
**Fix:** Food indices are deduplicated via `std::set<int, std::greater<int>>` before removal (`src/core/environment.cpp:203-212`).

---

## HIGH

### H1. ~~Training progress reports 0 food when no generation reset happens~~ **FIXED**
**Fix:** The environment now tracks `best_food_ever` as a continuous atomic counter updated every step (`src/core/environment.cpp:164-167`). Training progress reads this value instead of depending on `last_gen_best_food`.

### H2. `NeuralNetwork::set_genome()` no bounds check — out-of-bounds read
**File:** `src/core/neural_network.cpp:299-328`

```cpp
void NeuralNetwork::set_genome(const std::vector<double>& genome) {
    size_t index = 0;
    for (auto& layer : layers) {
        const auto& weights_ref = layer.get_weights();
        size_t weight_rows = weights_ref.size();
        size_t weight_cols = weight_rows > 0 ? weights_ref[0].size() : 0;
        // ...
        for (auto& w : weights) {
            for (auto& val : w) {
                val = genome[index++];  // OOB if genome too short
            }
        }
    }
}
```

If `genome.size()` does not exactly match the expected flattened size, `index` overruns the vector. Called from `AI::load_brain_from_json` and `Environment::reset` — a corrupted `brain.json` can cause UB or a crash.

### H3. NaN/Inf genome propagation through agent respawning
**File:** `src/core/environment.cpp:222-224`

When extracting the template genome from a dead agent for respawning, there is no NaN/Inf validation:

```cpp
if (!agent.is_alive() && !found_template) {
    template_genome = agent.get_neural_network()->get_genome();
    found_template = true;
}
```

If training produced a NaN/Inf weight (possible via gradient explosion despite clipping), the invalid values propagate to ALL respawned agents. **Mitigation:** `save_brain_to_json()` replaces NaN/Inf with 0.0 (`src/core/ai_agent.cpp:159-170`) and `load_brain_from_json()` rejects them (`src/core/ai_agent.cpp:186-190`), but the respawn code path bypasses save/load validation.

### H4. `NeuralNetwork::get_genome()` crash on empty weights
**File:** `src/core/neural_network.cpp:277-284`

```cpp
const auto& weights = layer.get_weights();
total_size += weights.size() * weights[0].size() + layer.get_biases().size();
```

If `weights.size() == 0` (empty weights matrix), `weights[0]` is undefined behaviour. Currently unreachable (all layers have `output_size > 0`) but latent.

---

## MEDIUM

### M1. `operator=` does not copy `train_step_counter` or `gen`/`dis`
**File:** `src/core/ai_agent.cpp:131-150`

```cpp
AI& AI::operator=(const AI& other) {
    if (this != &other) {
        pos = other.pos;
        energy = other.energy;
        // ... copies explore_rate, update_counter, last_q_values, last_actions,
        //     layer_sizes, networks
        // MISSING: train_step_counter, gen, dis
    }
    return *this;
}
```

The `train_step_counter` (controls DQN train interval) and RNG state (`gen`/`dis`) are not copied. After assignment, the new agent's RNG produces unpredictable sequences, and `train_step_counter` stays at its default 0, causing an immediate DQN training step even if the source agent wasn't due for one.

### M2. Sidebar food-history graph persists across visual-sim resets
**File:** `src/rendering/renderer.cpp:87-101`

When the user presses `R` in the visual simulation, `env.reset()` is called. The `agent_history` vector is **not** cleared — it is only cleared when agent count changes. Since `env.reset()` increments episode (e.g. 1 → 2), the new data appends to old data, visually blending multiple generations into one graph.

### M3. ~~Dead code: `select_parents()` and `crossover()`~~ **FIXED**
**Fix:** These methods were removed during refactoring. Evolution now uses `mutate_genome()` in place.

### M4. Duplicate food on same grid cell
**File:** `src/core/environment.cpp:31-40`

`spawn_food()` places food at random `(x, y)` coordinates without checking for existing food or agent occupancy. Multiple food items can occupy the same cell. Agents that land on such a cell eat all food there at once in a single step.

### M5. Duplicate agents on same grid cell
**File:** `src/core/environment.cpp:106-109`, `src/core/environment.cpp:243-246`

Agent placement (both `reset()` and respawn) places agents at random positions without overlap checking. Multiple agents can start on the same cell.

---

## LOW

### L1. ~~Visual-sim agent history capped at 16 agents~~ **FIXED**
**Fix:** The hardcoded 16-element array was replaced with `std::vector<std::vector<FoodPoint>>` (`src/rendering/renderer.cpp:15`). History limit is now `MAX_HISTORY = 500`.

### L2. Sidebar legend overflow on multi-agent
**File:** `src/rendering/renderer.cpp:348-363`

The legend line for "Best" + per-agent labels draws sequentially left-to-right. With >6-8 agents the labels overflow the sidebar width. The `lx + 22 > graph_x + graph_w` break prevents a visual glitch but drops agents from the legend silently.

### L3. ~~`GRID_HEIGHT` misleading — equals `SCREEN_HEIGHT`~~ **FIXED**
**Fix:** The `GRID_WIDTH`/`GRID_HEIGHT` constants were removed from `config.hpp`. The grid is now always drawn as a square.

### L4. ~~Mouse click not validated against actual grid extent~~ **FIXED**
**Fix:** The debug click handler now checks `mp.x < gpw && mp.y < gpw` where `gpw = cell_size * grid_size` (`src/main.cpp:80`), correctly restricting clicks to the grid area.

### L5. `copy_weights_from()` no shape compatibility check
**File:** `src/core/neural_network.cpp:253-258`

```cpp
void NeuralNetwork::copy_weights_from(const NeuralNetwork& other) {
    for (size_t i = 0; i < layers.size(); ++i) {
        layers[i].set_weights(other.layers[i].get_weights());
        layers[i].set_biases(other.layers[i].get_biases());
    }
}
```

If `layers.size()` or shapes differ (e.g. architectural change between saved brain and current config), this silently copies wrong data. In practice the structure is always identical for DQN target copies, but `set_genome` (called from `load_brain_from_json`) can supply a genome for a different architecture.

### L6. ~~`brain_save_mutex` function-local static~~ **FIXED**
**Fix:** The function-local static mutex was removed during refactoring. Brain saving now uses `last_saved_best_food_` as a threshold guard.

### L7. Food is not replenished when `cfg.food_count` changes
**File:** `src/core/environment.cpp:24-41`

`spawn_food()` only spawns enough food to reach `cfg.food_count`. If `cfg.food_count` is increased at runtime (via settings menu), the food count rises to meet the new target. But if `cfg.food_count` is **decreased**, existing food above the new count is never cleaned up and remains on the grid.

### L8. `food_reset_threshold` can be larger than `food_count`
**File:** `include/core/config.hpp:92`, `src/core/environment.cpp:43-47`

The settings menu allows `food_reset_threshold` up to 200 but `food_count` can be as low as 1. If `threshold > count`, `spawn_food()` is called every step (since `food_list.size()` is always `< threshold`), wasting CPU spawning 0 food (`to_spawn = count - size ≤ 0` → early return). Harmless but wasteful.

---

## TESTING ISSUES

### T1. `test_neural_network.cpp` does not test `set_genome` with wrong-sized genome
**File:** `tests/test_neural_network.cpp`

The genome-size mismatch test is still missing. A corrupted `brain.json` triggers H2 (OOB read).

### T2. No integration test for food consumption or agent respawning
**Files:** (missing)

The test suite only covers neural network forward/copy/train. There are no tests for:
- Food consumption mechanics
- Agent respawning
- Generation resets
- Parallel step execution correctness
- Brain save/load round-trip

### T3. No test for cross-platform DPI scaling
**Files:** (missing)

The `os_scale()` function uses `GetRenderWidth()/GetScreenWidth()` as the primary DPI source with a `GetWindowScaleDPI()` fallback. There is no automated test to verify the scale factor is correct across platforms.

---

## SUMMARY BY FILE

| File | Bugs |
|---|---|
| `src/main.cpp` | — |
| `src/menu/menu.cpp` | — |
| `src/core/environment.cpp` | H3, M4, M5, L7, L8 |
| `src/core/ai_agent.cpp` | M1 |
| `src/core/neural_network.cpp` | H2, H4, L5 |
| `src/rendering/renderer.cpp` | M2, L2 |
| `include/core/config.hpp` | L8 |
| `include/rendering/renderer.hpp` | — (T3) |
| `tests/test_neural_network.cpp` | T1, T2 |

## RESOLVED

| ID | Bug | Fix |
|---|---|---|
| C1 | `std::terminate` on second training | Join thread before reassign |
| C2 | Food double-erase corruption | Dedup indices with set |
| H1 | Training progress 0 food | Added `best_food_ever` atomic |
| M3 | Dead code `select_parents`/`crossover` | Removed during refactor |
| L1 | Agent history capped at 16 | Replaced fixed array with vector |
| L3 | `GRID_HEIGHT` misleading | Removed constants |
| L4 | Click not validated against grid | Use `gpw` for both bounds |
| L6 | Function-local `brain_save_mutex` | Removed during refactor |
| — | DPI scaling inconsistent across platforms | `os_scale()` uses render ratio with fallback |
| — | Window size inconsistent across platforms | Init size scaled to monitor |
