#ifndef MACROBLOCK_H
#define MACROBLOCK_H

#include <iostream>
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
    std::vector<std::tuple<int, int, int>> motion_vectors;

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
    Block(unsigned int size, Pos pos, unsigned int split, std::vector<std::tuple<int, int, int>> motion_vectors)
        : size(size)
        , pos(pos)
        , split(split)
        , motion_vectors(std::move(motion_vectors))
    {
    }
};

struct Macroblock : Block {
    std::optional<std::array<Block, 4>> blocks = std::nullopt;
    MacroblockType type = S;

private:
    static Pos scaled_pos(const Pos& pos)
    {
        return Pos({ pos.x * MACROBLOCK_SIZE, pos.y * MACROBLOCK_SIZE });
    }

    void init_sub_blocks(const Pos parent_pos, unsigned int sub_split, const std::vector<std::vector<std::tuple<int, int, int>>>& vectors = {})
    {
        auto& bs = blocks.emplace();
        for (unsigned int i = 0; i < 2; ++i) {
            for (unsigned int j = 0; j < 2; ++j) {
                const unsigned int idx = i * 2 + j;
                auto sub_pos = Pos({ parent_pos.x * MACROBLOCK_SIZE + j * sub_size, parent_pos.y * MACROBLOCK_SIZE + i * sub_size });
                if (vectors.empty()) {
                    bs[idx] = Block(sub_size, sub_pos, sub_split & SUBSPLIT_MASK);
                } else {
                    bs[idx] = Block(sub_size, sub_pos, sub_split & SUBSPLIT_MASK, vectors[idx]);
                }
                sub_split >>= 2;
            }
        }
    }

    static void validate_vectors(const std::vector<std::vector<std::tuple<int, int, int>>>& vectors, size_t outer, size_t inner = -1) {
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
    explicit Macroblock(MacroblockType type, Pos pos, const std::vector<std::vector<std::tuple<int, int, int>>>& motion_vectors)
        : Block(MACROBLOCK_SIZE, scaled_pos(pos), 3)
        , type(type)
    {
        if (type != S) {
            throw std::invalid_argument("Invalid arguments for Macroblock S");
        }
        if (motion_vectors.empty() || motion_vectors.front().empty()) {
            throw std::invalid_argument("Invalid vectors for Macroblock S");
        }
        for (int i = 0; i < 4; i++) {
            this->motion_vectors.push_back(motion_vectors.front().front());
        }
    }

    // Constructor for `BlockType::I` (Intra-coded)
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        unsigned int sub_split = 0)
        : Block(MACROBLOCK_SIZE, scaled_pos(pos), split)
        , type(type)
    {
        if (type != I) {
            throw std::invalid_argument("Invalid type for Macroblock I");
        }

        if (split == 1 || split == 2) {
            throw std::invalid_argument("Invalid splits for type I");
        }

        if (split == 3 && sub_split == FULL_SUBSPLIT) {
            init_sub_blocks(pos, sub_split);
        } else if (split == 3 && sub_split != 0) {
            throw std::invalid_argument("Invalid sub splits for type I");
        }
    }

    // Constructor for `BlockType::P` (Predicted) with motion vectors
    explicit Macroblock(MacroblockType type,
        Pos pos,
        unsigned int split,
        unsigned int sub_split,
        const std::vector<std::vector<std::tuple<int, int, int>>>& motion_vectors)
        : Block(MACROBLOCK_SIZE, scaled_pos(pos), split)
        , type(type)
    {
        if (type != P) {
            throw std::invalid_argument("Invalid type for Macroblock P");
        }
        if (split == 0) {
            validate_vectors(motion_vectors, 1, 1);
            this->motion_vectors.push_back(motion_vectors[0][0]);
        } else if (split == 1 || split == 2) {
            validate_vectors(motion_vectors, 1, 2);
            this->motion_vectors.push_back(motion_vectors[0][0]);
            this->motion_vectors.push_back(motion_vectors[0][1]);
        } else if (split == 3) {
            if (sub_split == 0) {
                for (int i = 0; i < 4; ++i) {
                    this->motion_vectors.push_back(motion_vectors[0][i]);
                }
            } else {
                validate_vectors(motion_vectors, 4);

                init_sub_blocks(pos, sub_split, motion_vectors);
            }
        } else {
            throw std::invalid_argument("Invalid split value for Macroblock P");
        }
    }
};
#endif //MACROBLOCK_H
