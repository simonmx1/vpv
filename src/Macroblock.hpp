#ifndef MACROBLOCK_H
#define MACROBLOCK_H
struct Macroblock {
    int x;
    int y;
    int width;
    int height;
    int split; // How the Macroblock is split. 0 = not split, 1 = vertically, 2 = horizontally, 3 = both
    std::array<int, 4> sub_split;
    char type;
    static void parse_sub_splits(std::array<int, 4>& sub_splits, int sub_split_int)
    {
        for (int i = 0; i < 4; i++) {
            sub_splits[i] = sub_split_int & 3;
            sub_split_int >>= 2;
        }
    }
};
#endif //MACROBLOCK_H
