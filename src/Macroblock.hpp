#ifndef MACROBLOCK_H
#define MACROBLOCK_H

#include "imgui.h"

#include <array>
#include <functional>
#include <optional>
#include <vector>

constexpr unsigned int MACROBLOCK_SIZE = 16;
constexpr unsigned int SUBSPLIT_MASK = 0b11; // 3
constexpr unsigned int FULL_SUBSPLIT = 0b11111111; // 255, 2 bits each represent the subsplits (0 to 3)
constexpr unsigned int SUB_SIZE = MACROBLOCK_SIZE / 2;

struct Pos {
    unsigned int x = 0, y = 0;
    explicit Pos(std::array<unsigned int, 2> pos)
        : x(pos[0])
        , y(pos[1])
    {
    }
};

enum MacroblockType {
    S,
    I,
    P,
    B
};

class Block {
public:
    unsigned int size = 0;
    unsigned int split = 0;
    std::vector<std::tuple<int, int, int>> motionVectors;
    std::vector<unsigned int> modes;

    Block();
    Block(unsigned int size, Pos pos, unsigned int split);
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<std::tuple<int, int, int>> motionVectors);
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<unsigned int> modes);

    // TODO check if c++17 and look into [[nodiscard]] annotations
    // pos and size access through getters to provide float conversions for ImGui but stored as int, because of pixels
    float getX() const { return static_cast<float>(pos.x); }
    float getY() const { return static_cast<float>(pos.y); }
    float getSize() const { return static_cast<float>(size); }
    ImVec2 getTopLeft() const { return { getX(), getY() }; }
    ImVec2 getBottomRight() const { return { getX() + getSize(), getY() + getSize() }; }

private:
    Pos pos;
};

/*
 * Design choice explanation:
 * Once a block has split mode 3, it automatically creates 4 subblocks,
 * because it might have 8 vectors in a B-block. This can't be stored
 * in a single Macroblock without subblocks, since it can hold a maximum
 * of 4 vectors. Since B blocks do not have sub-splits,
 * there is no necessity to have more than 16 vectors in a macroblock ever.
 */
class Macroblock : public Block {
public:
    std::optional<std::array<Block, 4>> blocks = std::nullopt;
    MacroblockType type = S;

    // Constructor for `BlockType::S` (default)
    explicit Macroblock(MacroblockType type, Pos pos, const std::tuple<int, int, int>& motionVector);

    // Constructor for `BlockType::I` (Intra-coded)
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        const std::vector<std::vector<unsigned int>>& modes,
        unsigned int subSplit = 0);

    // Constructor for `BlockType::P` (Predicted) with motion vectors
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        unsigned int subSplit,
        const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors);

    // Constructor for `BlockType::B` with motion vectors
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors);

private:
    static Pos getScaledPos(const Pos& pos);
    void initSubBlocksCommon(const Pos& parent_pos, unsigned int subSplit, const std::function<Block(unsigned int, Pos, unsigned int, unsigned int)>& blockConstructor);

    void initSubBlocks(const Pos& parent_pos, unsigned int subSplit, const std::vector<std::vector<std::tuple<int, int, int>>>& vectors = {});
    void initSubBlocks(const Pos& parent_pos, unsigned int subSplit, const std::vector<std::vector<unsigned int>>& modes);

    static void validateVectors(const std::vector<std::vector<std::tuple<int, int, int>>>& vectors, size_t outer, std::optional<size_t> inner);
    static void validateModes(const std::vector<std::vector<unsigned int>>& modes, unsigned int split, unsigned int subSplit);

};
#endif //MACROBLOCK_H
