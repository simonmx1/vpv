#ifndef MACROBLOCK_H
#define MACROBLOCK_H

#include <optional>
#include <utility>
#include <variant>
#include <vector>

constexpr unsigned int MACROBLOCK_SIZE = 16;
constexpr unsigned int SUBSPLIT_MASK = 0b11; // 3
constexpr unsigned int FULL_SUBSPLIT = 0b11111111; // 255

constexpr unsigned int sub_size = MACROBLOCK_SIZE / 2;

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

struct Block {
    unsigned int size = 0;
    Pos pos;
    unsigned int split = 0;
    std::vector<std::tuple<int, int, int>> motionVectors;

    Block()
        : pos(Pos({ 0, 0 }))
    {
    }

    Block(unsigned int size, Pos pos, unsigned int split)
        : size(size)
        , pos(pos)
        , split(split)
    {
    }
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<std::tuple<int, int, int>> motionVectors)
        : size(size)
        , pos(pos)
        , split(split)
        , motionVectors(std::move(motionVectors))
    {
    }
};

struct Macroblock : Block {
    std::optional<std::array<Block, 4>> blocks = std::nullopt;
    MacroblockType type = S;

private:
    static Pos scaledPos(const Pos& pos)
    {
        return Pos({ pos.x * MACROBLOCK_SIZE, pos.y * MACROBLOCK_SIZE });
    }

    void initSubBlocks(const Pos parent_pos, unsigned int subSplit, const std::vector<std::vector<std::tuple<int, int, int>>>& vectors = {})
    {
        auto& bs = blocks.emplace();
        for (unsigned int i = 0; i < 2; ++i) {
            for (unsigned int j = 0; j < 2; ++j) {
                const unsigned int idx = i * 2 + j;
                auto sub_pos = Pos({ parent_pos.x * MACROBLOCK_SIZE + j * sub_size, parent_pos.y * MACROBLOCK_SIZE + i * sub_size });
                if (vectors.empty()) {
                    bs[idx] = Block(sub_size, sub_pos, subSplit & SUBSPLIT_MASK);
                } else {
                    bs[idx] = Block(sub_size, sub_pos, subSplit & SUBSPLIT_MASK, vectors[idx]);
                }
                subSplit >>= 2;
            }
        }
    }

    static void validateVectors(const std::vector<std::vector<std::tuple<int, int, int>>>& vectors, size_t outer, size_t inner = -1) {
        // May seem like unreachable code right now, but might produce segmentation fault later
        // ReSharper disable once CppDFAConstantConditions
        if (vectors.empty() && outer == 0) {
            // ReSharper disable once CppDFAUnreachableCode
            return;
        }
        if (vectors.size() != outer || inner != -1 || vectors[0].size() != inner) {
            throw std::invalid_argument("Invalid motion vector dimensions");
        }
    }

public:
    // Constructor for `BlockType::S` (default)
    explicit Macroblock(MacroblockType type, Pos pos, const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors)
        : Block(MACROBLOCK_SIZE, scaledPos(pos), 3)
        , type(type)
    {
        if (type != S) {
            throw std::invalid_argument("Invalid arguments for Macroblock S");
        }
        if (motionVectors.empty() || motionVectors.front().empty()) {
            throw std::invalid_argument("Invalid vectors for Macroblock S");
        }
        for (int i = 0; i < 4; i++) {
            this->motionVectors.push_back(motionVectors.front().front());
        }
    }

    // Constructor for `BlockType::I` (Intra-coded)
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        unsigned int subSplit = 0)
        : Block(MACROBLOCK_SIZE, scaledPos(pos), split)
        , type(type)
    {
        if (type != I) {
            throw std::invalid_argument("Invalid type for Macroblock I");
        }

        if (split == 1 || split == 2) {
            throw std::invalid_argument("Invalid splits for type I");
        }

        if (split == 3 && subSplit == FULL_SUBSPLIT) {
            initSubBlocks(pos, subSplit);
        } else if (split == 3 && subSplit != 0) {
            throw std::invalid_argument("Invalid sub splits for type I");
        }
    }

    // Constructor for `BlockType::P` (Predicted) with motion vectors
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        unsigned int subSplit,
        const std::vector<std::vector<std::tuple<int, int, int>>>& motionVectors)
        : Block(MACROBLOCK_SIZE, scaledPos(pos), split)
        , type(type)
    {
        if (type != P) {
            throw std::invalid_argument("Invalid type for Macroblock P");
        }
        if (split == 0) {
            validateVectors(motionVectors, 1, 1);
            this->motionVectors.push_back(motionVectors[0][0]);
        } else if (split == 1 || split == 2) {
            validateVectors(motionVectors, 1, 2);
            this->motionVectors.push_back(motionVectors[0][0]);
            this->motionVectors.push_back(motionVectors[0][1]);
        } else if (split == 3) {
            if (subSplit == 0) {
                for (int i = 0; i < 4; ++i) {
                    this->motionVectors.push_back(motionVectors[0][i]);
                }
            } else {
                validateVectors(motionVectors, 4);

                initSubBlocks(pos, subSplit, motionVectors);
            }
        } else {
            throw std::invalid_argument("Invalid split value for Macroblock P");
        }
    }
};
#endif //MACROBLOCK_H
