/**
 * @file experience.hpp
 * @brief Defines the Experience struct for reinforcement learning experience replay.
 * 
 * Experience replay is a technique where the agent stores its experiences
 * (state, action, reward, next_state, done) in a buffer and samples from
 * it randomly during training. This breaks correlations between consecutive
 * experiences and leads to better training stability.
 */

#ifndef EXPERIENCE_HPP
#define EXPERIENCE_HPP

#include <vector>

/**
 * @brief Represents a single experience tuple for reinforcement learning.
 * 
 * This struct stores all information needed to learn from a single
 * interaction with the environment:
 * - The state before taking the action
 * - Which action was taken
 * - The reward received
 * - The resulting new state
 * - Whether the episode terminated after this action
 */
struct Experience {
    std::vector<double> state;       ///< State representation before action
    int action;                        ///< Action taken (0-3: LEFT, RIGHT, UP, DOWN)
    double reward;                      ///< Reward received
    std::vector<double> next_state;    ///< State representation after action
    bool done;                          ///< Whether the episode ended (agent died)
};

#endif // EXPERIENCE_HPP
