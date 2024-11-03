#ifndef MACROBLOCK_H
#define MACROBLOCK_H
struct Macroblock {
    int x;
    int y;
    int width;
    int height;
    int split;  // How the Macroblock is split. 0 = not split, 1 = vertically, 2 = horizontally, 3 = both
};
#endif //MACROBLOCK_H
