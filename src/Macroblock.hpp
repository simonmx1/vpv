#ifndef MACROBLOCK_H
#define MACROBLOCK_H

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
    Pos pos;
    unsigned int split = 0;
    std::vector<std::tuple<int, int, int>> motionVectors;
    std::vector<unsigned int> modes;

    Block();
    Block(unsigned int size, Pos pos, unsigned int split);
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<std::tuple<int, int, int>> motionVectors);
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<unsigned int> modes);
};

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
