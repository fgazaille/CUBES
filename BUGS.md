# Bug Report — CUBES Simulation

## CRITICAL

### C1. `std::terminate` crash on second training session (`main.cpp` / `menu.cpp`)
**Files:** `src/main.cpp:163-236`, `src/menu/menu.cpp:180-185`

When training finishes (agents reach 0 `episodes_done`), `do_training_active()` returns state to `HOME`. The training thread exits its task function but `TrainingStatus::thread` remains **joinable**. If the user starts a second training session via `TrainingConfig` → "Start Training", `run_training()` assigns `status->thread = std::thread(...)`, destroying the still-joinable previous `thread` object, which calls `std::terminate()`.

**Trigger sequence:**
1. Start training → let it finish → state returns to HOME menu
2. Training thread completed but `thread` object never joined
3. Click "Training" → "Start Training" → `run_training()` → new `std::thread` assigned → **crash**

**Fix:** Join the old thread in `run_training()` before reassigning, or join it in `menu.run()` when leaving `TRAINING_ACTIVE` state.

### C2. Same-index food double-erase corrupts food list when two agents eat same food in one step (`environment.cpp`)
**Files:** `src/core/environment.cpp:218-266`

Each agent task copies `food_list` locally, records the `(agent_idx, local_food_index)` pair on consumption, and the task processes the local copy. After the thread pool drains, the recorded indices are sorted descending and erased from the real `food_list`:

```cpp
std::sort(food_consumed.rbegin(), food_consumed.rend(),
          [](const auto& a, const auto& b) { return a.second > b.second; });
for (const auto& consumed : food_consumed) {
    if (consumed.second >= 0 && consumed.second < static_cast<int>(food_list.size()))
        food_list.erase(food_list.begin() + consumed.second);
}
```

If **two agents eat from the same food item** in the same step (both land on the same grid cell that has food — there is no agent-grid-collision prevention), both record the same `local_food` index. The first `erase` removes that food, shifting all subsequent foods down by one. The second `erase` with the same index removes the **wrong** food.

Same bug also triggers if two different food items happen to be at distinct indices that **collide after shifting** during sequential removal.

**Fix:** Deduplicate `food_consumed` by index (e.g. `std::set<int>`) or track consumption by food position rather than index.

---

## HIGH

### H1. Training progress reports 0 food when no generation reset happens (`main.cpp` / `environment.cpp`)
**Files:** `src/main.cpp:198`, `src/core/environment.cpp:300-314`

`Environment::get_last_gen_best_food()` returns the `last_gen_best_food` field, which is **only updated inside `reset()`** (the full generation-level reset). With the new individual per-step agent respawning logic, agents can run for thousands of steps (and eat food) without ever triggering `reset()` (which requires `alive_count == 0` at the start of respawn). The training progress graph stays at `best_food: 0` until a generation-level reset happens, even though agents are actively eating.

### H2. `NeuralNetwork::set_genome()` no bounds check — out-of-bounds read (`neural_network.cpp`)
**Files:** `src/core/neural_network.cpp:299-328`

```cpp
void NeuralNetwork::set_genome(const std::vector<double>& genome) {
    size_t index = 0;
    for (auto& layer : layers) {
        // ... 
        for (auto& w : weights)
            for (auto& val : w)
                val = genome[index++];  // OOB if genome too short
        for (auto& b : biases)
            b = genome[index++];  // OOB if genome too short
    }
}
```

If `genome.size()` does not exactly match the expected flattened size, `index` overruns the vector. Called from `AI::load_brain_from_json` and `Environment::reset` — a corrupted `brain.json` can cause UB or a crash.

### H3. NaN/Inf genome propagation through agent respawning (`environment.cpp`)
**Files:** `src/core/environment.cpp:292-303`

When extracting the template genome from a dead agent for respawning, there is no NaN/Inf validation:

```cpp
if (!agent.is_alive() && !found_template) {
    template_genome = agent.get_neural_network()->get_genome();
    found_template = true;
}
```

If training produced a NaN/Inf weight (possible via gradient explosion despite clipping), the invalid values propagate to ALL respawned agents, potentially corrupting the entire population in one step.

### H4. `NeuralNetwork::get_genome()` crash on empty weights (`neural_network.cpp`)
**Files:** `src/core/neural_network.cpp:277-284`

```cpp
const auto& weights = layer.get_weights();
total_size += weights.size() * weights[0].size() + layer.get_biases().size();
```

If `weights.size() == 0` (empty weights matrix), `weights[0]` is undefined behaviour. Currently unreachable (all layers have `output_size > 0`) but latent.

---

## MEDIUM

### M1. `operator=` does not copy `train_step_counter` or `gen`/`dis` (`ai_agent.cpp`)
**Files:** `src/core/ai_agent.cpp:98-108`

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

### M2. Sidebar food-history graph persists across visual-sim resets (`renderer.cpp`)
**Files:** `src/rendering/renderer.cpp:19-23, 107-123`

When the user presses `R` in the visual simulation, `env.reset()` is called. The `agent_history[]` array is **not** cleared — it is only cleared when `ep < last_recorded_episode` (a new-simulation condition) or when agent count changes. Since `env.reset()` increments episode (e.g. 1 → 2), the new data appends to old data, visually blending multiple generations into one graph.

### M3. Dead code: `select_parents()` and `crossover()` (`environment.hpp` / `environment.cpp`)
**Files:** `include/core/environment.hpp:62-81`, `src/core/environment.cpp:100-127`

The `select_parents()` and `crossover()` methods are declared, defined, and documented but **never called**. `reset()` now uses the simpler template-genome-with-mutation approach. This is dead code that adds confusion.

### M4. Duplicate food on same grid cell (`environment.cpp`)
**Files:** `src/core/environment.cpp:110-120`

`spawn_food()` places food at random `(x, y)` coordinates without checking for existing food or agent occupancy. Multiple food items can occupy the same cell. Agents that land on such a cell eat all food there at once in a single step.

### M5. Duplicate agents on same grid cell (`ai_agent.cpp` / `environment.cpp`)
**Files:** `src/core/ai_agent.cpp:38-39`, `src/core/environment.cpp:141-145`

Agent spawning (both constructor and `reset()`) places agents at random positions without overlap checking. Multiple agents can start on the same cell.

---

## LOW

### L1. Visual-sim agent history capped at 16 agents (`renderer.cpp`)
**Files:** `src/rendering/renderer.cpp:15`

```cpp
std::vector<FoodPoint> agent_history[16];
```

Hardcoded 16-element array. If `agent_count > 16`, agents beyond index 15 are silently omitted from the sidebar graph. The `i < 16` guard prevents overflow but loses data.

### L2. Sidebar legend overflow on multi-agent (`renderer.cpp`)
**Files:** `src/rendering/renderer.cpp:322-337`

The legend line for "Best" + per-agent labels draws sequentially left-to-right. With >6-8 agents the labels overflow the sidebar width. The `lx + 22 > graph_x + graph_w` break prevents a visual glitch but drops agents from the legend silently.

### L3. `GRID_HEIGHT` misleading — equals `SCREEN_HEIGHT`, never used as grid extent (`config.hpp`)
**Files:** `include/core/config.hpp:63`

```cpp
inline int GRID_WIDTH = SCREEN_WIDTH - SIDEBAR_WIDTH;
inline int GRID_HEIGHT = SCREEN_HEIGHT;
```

`GRID_HEIGHT` is the full screen height, not the grid's visual height. The grid is always drawn as a square (`cell_size * grid_size` on each side) at `renderer.cpp:126`. `GRID_HEIGHT` is only used for mouse click bounds in `main.cpp:86` — clicking anywhere in the full vertical extent of the window (including below the grid) is accepted as a grid click.

### L4. Mouse click not validated against actual grid extent (`main.cpp`)
**Files:** `src/main.cpp:84-99`

```cpp
if (mp.x < GRID_WIDTH && mp.y < GRID_HEIGHT) {
    int cx = (int)mp.x / CELL_SIZE;
    int cy = (int)mp.y / CELL_SIZE;
```

`GRID_HEIGHT == SCREEN_HEIGHT`, not the grid's visual height (`cell_size * grid_size`). A click below the grid (e.g. y=850 on a 900px window with a 700px grid) passes the bounds check. `cy` can index a non-existent cell row.

### L5. `copy_weights_from()` no shape compatibility check (`neural_network.cpp`)
**Files:** `src/core/neural_network.cpp:253-258`

```cpp
void NeuralNetwork::copy_weights_from(const NeuralNetwork& other) {
    for (size_t i = 0; i < layers.size(); ++i) {
        layers[i].set_weights(other.layers[i].get_weights());
        layers[i].set_biases(other.layers[i].get_biases());
    }
}
```

If `layers.size()` or shapes differ (e.g. architectural change between saved brain and current config), this silently copies wrong data. In practice the structure is always identical for DQN target copies, but `set_genome` (called from `load_brain_from_json`) can supply a genome for a different architecture.

### L6. `brain_save_mutex` function-local static in `reset()` (`environment.cpp`)
**Files:** `src/core/environment.cpp:158`

```cpp
static std::mutex brain_save_mutex;
std::lock_guard<std::mutex> lock(brain_save_mutex);
```

Function-local `static` mutex is unnecessary in single-environment mode. If two `Environment` instances somehow called `reset()` concurrently (e.g. a future refactor), the mutex would be shared across all instances, which is correct but fragile. Better to make it a member.

### L7. Food is not replenished when `cfg.food_count` changes (`environment.cpp`)
**Files:** `src/core/environment.cpp:110-120`

`spawn_food()` only spawns enough food to reach `cfg.food_count`. If `cfg.food_count` is increased at runtime (via `--repl` param or settings menu), the food count rises to meet the new target. But if `cfg.food_count` is **decreased**, existing food above the new count is never cleaned up and remains on the grid.

### L8. `food_reset_threshold` can be larger than `food_count` (`config.hpp` / `menu.cpp`)
**Files:** `include/core/config.hpp:102`, `src/menu/menu.cpp:360`

The settings menu allows `food_reset_threshold` up to 100 but `food_count` can be as low as 1. If `threshold > count`, `spawn_food()` is called every step (since `food_list.size()` is always `< threshold`), wasting CPU spawning 0 food (`to_spawn = count - size ≤ 0` → early return). Harmless but wasteful.

---

## TESTING ISSUES

### T1. `test_neural_network.cpp` does not test `set_genome` with wrong-sized genome (`tests/test_neural_network.cpp`)
**Files:** `tests/test_neural_network.cpp:27-44`

The `test_network_genome` test gets a genome from the source network and sets it on a target with the same architecture. There is no test for loading a genome of incorrect size (which would trigger H2/OOB-read).

### T2. No integration test for food consumption or agent respawning
**Files:** (missing)

The test suite only covers neural network forward/copy/train. There are no tests for:
- Food consumption mechanics
- Agent respawning
- Generation resets
- Parallel step execution correctness
- Brain save/load round-trip

---

## SUMMARY BY FILE

| File | Bugs |
|---|---|
| `src/main.cpp` | C1, H1, L4 |
| `src/menu/menu.cpp` | C1 |
| `src/core/environment.cpp` | C2, H1, H3, M3, M4, M5, L6, L7 |
| `src/core/ai_agent.cpp` | M1, M5 |
| `src/core/neural_network.cpp` | H2, H4, L5 |
| `src/rendering/renderer.cpp` | M2, L1, L2 |
| `include/core/config.hpp` | L3, L8 |
| `tests/test_neural_network.cpp` | T1, T2 |
