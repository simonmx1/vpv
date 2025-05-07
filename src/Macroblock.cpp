#include "Macroblock.hpp"

#include <functional>
#include <stdexcept>
#include <tuple>

// Block constructors
Block::Block()
    : pos(Pos({ 0, 0 }))
{
}

Block::Block(unsigned int size, Pos pos, unsigned int split)
    : size(size)
    , pos(pos)
    , split(split)
{
}
Block::Block(unsigned int size, Pos pos, unsigned int split, std::vector<std::tuple<int, int, int>> motionVectors)
    : size(size)
    , pos(pos)
    , split(split)
    , motionVectors(std::move(motionVectors))
{
}
Block::Block(unsigned int size, Pos pos, unsigned int split, std::vector<unsigned int> modes)
    : size(size)
    , pos(pos)
    , split(split)
    , modes(std::move(modes))
{
}

// Private methods
Pos Macroblock::getScaledPos(const Pos& pos)
{
    return Pos({ pos.x * MACROBLOCK_SIZE, pos.y * MACROBLOCK_SIZE });
}

// TODO: look into template functions maybe
void Macroblock::initSubBlocksCommon(const Pos& parent_pos, unsigned int subSplit, const std::function<Block(unsigned int, Pos, unsigned int, unsigned int)>& blockConstructor)
{
    auto& bs = blocks.emplace();
    for (unsigned int i = 0; i < 2; ++i) {
        for (unsigned int j = 0; j < 2; ++j) {
            const unsigned int idx = 3 - (i * 2 + j);
            auto sub_pos = Pos({ parent_pos.x * MACROBLOCK_SIZE + j * SUB_SIZE,
                parent_pos.y * MACROBLOCK_SIZE + i * SUB_SIZE });

            bs[idx] = blockConstructor(idx, sub_pos, subSplit & SUBSPLIT_MASK, idx);

            subSplit >>= 2;
        }
    }
}

void Macroblock::initSubBlocks(const Pos& parent_pos, const unsigned int subSplit,
    const std::vector<std::vector<std::tuple<int, int, int>>>& vectors)
{
    initSubBlocksCommon(parent_pos, subSplit, [&](unsigned int idx, const Pos& sub_pos, unsigned int splitBits, unsigned int) {
        if (vectors.empty()) {
            return Block(SUB_SIZE, sub_pos, splitBits);
        }
        return Block(SUB_SIZE, sub_pos, splitBits, vectors[idx]);
    });
}

void Macroblock::initSubBlocks(const Pos& parent_pos, const unsigned int subSplit, const std::vector<std::vector<unsigned int>>& modes)
{
    initSubBlocksCommon(parent_pos, subSplit, [&](unsigned int idx, const Pos& sub_pos, unsigned int splitBits, unsigned int) {
        return Block(SUB_SIZE, sub_pos, splitBits, modes[idx]);
    });
}

void Macroblock::validateVectors(const std::vector<std::vector<std::tuple<int, int, int>>>& vectors, size_t outer, std::optional<size_t> inner)
{
    // May seem like unreachable code right now, but might produce segmentation fault later
    if (vectors.empty() && outer == 0) {
        return;
    }
    if (vectors.size() != outer || inner.has_value() && vectors[0].size() != inner) {
        throw std::invalid_argument("Invalid motion vector dimensions!");
    }
}

void Macroblock::validateModes(const std::vector<std::vector<unsigned int>>& modes, unsigned int split, unsigned int subSplit)
{
    if (modes.empty()) {
        throw std::invalid_argument("Invalid empty mode vector for block I!");
    }

    bool isValid = false;

    if (split == 0) {
        // For split=0, e.g. [[1]]
        isValid = modes.size() == 1 && modes.front().size() == 1;
    } else if (split == 3) {
        if (subSplit == 0) {
            // For split=3, subSplit=0, e.g. [[1][1][1][1]]
            isValid = modes.size() == 4;
            // TODO: CHECK IF C++20 is available to replace lame for loops with the new std::ranges
            for (const auto& m : modes) {
                isValid &= m.size() == 1;
            }
        } else if (subSplit == 255) {
            // For split=3, subSplit=255, vector should have 4 vectors with 4 elements each.
            isValid = modes.size() == 4;
            for (const auto& m : modes) {
                isValid &= m.size() == 4;
            }
        }
    }

    if (!isValid) {
        throw std::invalid_argument("Invalid mode vector dimensions for block I!");
    }
}

// Macroblock constructors

Macroblock::Macroblock(MacroblockType type, Pos pos, const std::tuple<int, int, int>& motionVector)
    : Block(MACROBLOCK_SIZE, getScaledPos(pos), 3)
    , type(type)
{
    if (type != S) {
        throw std::invalid_argument("Invalid type for Macroblock S structure");
    }
    this->motionVectors.push_back(motionVector);
}

Macroblock::Macroblock(MacroblockType type,
    Pos pos,
    unsigned int split,
    const std::vector<std::vector<unsigned int>>& modes,
    unsigned int subSplit)
    : Block(MACROBLOCK_SIZE, getScaledPos(pos), split)
    , type(type)
{
    if (type != I) {
        throw std::invalid_argument("Invalid type for Macroblock I structure");
    }

    if (split == 1 || split == 2) {
        throw std::invalid_argument("Invalid splits for type I");
    }

    validateModes(modes, split, subSplit);
    if (split == 0) {
        this->modes = modes.front();
    } else {
        if (subSplit != 0 && subSplit != 255) {
            throw std::invalid_argument("Invalid sub splits for type I");
        }
        initSubBlocks(pos, subSplit, modes);
    }
}

Macroblock::Macroblock(MacroblockType type, Pos pos, unsigned int split, unsigned int subSplit, const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors)
    : Block(MACROBLOCK_SIZE, getScaledPos(pos), split)
    , type(type)
{
    if (type != P) {
        throw std::invalid_argument("Invalid type for Macroblock P structure");
    }

    switch (split) {
    case 0: {
        validateVectors(motionVectors, 1, 1);
        this->motionVectors.push_back(motionVectors[0][0]);
        break;
    }
    case 1:
        //TODO CHECK IF C++17 [[fallthrough]];
    case 2: {
        validateVectors(motionVectors, 1, 2);
        this->motionVectors.push_back(motionVectors[0][0]);
        this->motionVectors.push_back(motionVectors[0][1]);
        break;
    }
    case 3: {
        if (subSplit == 0) {
            initSubBlocks(pos, subSplit, motionVectors);
        } else {
            validateVectors(motionVectors, 4, std::nullopt);

            initSubBlocks(pos, subSplit, motionVectors);
        }
        break;
    }
    default:
        throw std::invalid_argument("Invalid split value for Macroblock P");
    }
}

Macroblock::Macroblock(MacroblockType type, Pos pos, unsigned int split, const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors)
    : Block(MACROBLOCK_SIZE, getScaledPos(pos), split)
    , type(type)
{
    if (type != B) {
        throw std::invalid_argument("Invalid type for Macroblock B structure");
    }
    switch (split) {
    case 0: {
        // TODO: Fix validation with variable length 1 or 2
        this->motionVectors.push_back(motionVectors[0][0]);
        break;
    }
    case 1:
        //TODO CHECK IF C++17 [[fallthrough]];
    case 2: {
        // TODO: Fix validation with variable length 1 or 2
        this->motionVectors.push_back(motionVectors[0][0]);
        if (std::get<2>(motionVectors[0][0]) > 1) {
            this->motionVectors.push_back(motionVectors[0][1]);
        } else {
            this->motionVectors.emplace_back(0, 0, 0);
        }
        this->motionVectors.push_back(motionVectors[1][0]);
        if (std::get<2>(motionVectors[1][0]) > 1) {
            this->motionVectors.push_back(motionVectors[1][1]);
        } else {
            this->motionVectors.emplace_back(0, 0, 0);
        }
        break;
    }
    case 3: {
        initSubBlocks(pos, 0, motionVectors);
        break;
    }
    default:
        throw std::invalid_argument("Invalid split value for Macroblock B");
    }
}