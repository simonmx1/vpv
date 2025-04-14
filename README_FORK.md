# This is the README file for the additions created in my fork

## Support for macroblocks
When using vpv via command line it is possible to add macroblock metadata to the image file.
This is done by adding metadata:data.json after the image like so: `vpv image.png metadata:image.json`.

### CAUTION
This is a test project and this only works on a single image.
I have not at all tested any edge cases or really any scenarios other than the one presented above.

## The format
I have currently chosen this format to pass the metadata:
```json
{
  "metadata": [
    {
      "x": 0,
      "y": 0,
      "width": 16,
      "height": 16,
      "split": 0
    },
    {
      "x": 0,
      "y": 16,
      "width": 16,
      "height": 16,
      "split": 2
    },
    ...
  ]
}
```
The x and y coordinates reference pixels on the image and give the TOP-LEFT corner of the macroblock.
The macroblock is as big as the data indicates and can be split in half again.
The split works like this:
```
0 = no split
1 = vertical
2 = horizontal
3 = both
```

## GUI interaction
To toggle the macroblock grid on the image, press m.


## Block types
The macroblock may contain a field "type" with values "I", "P", "S" or "B".
By pressing n, a layer of transparent color will be added over the frame to show the types in the editor.


## Subdivisions
If split is 3 (both directions), each subblock can again be split using vertical, horizontal or both lines.
This is indicated by the field "sub_split" and as an integer.
The integer comes from the binary number of 8 bits, which represent the subsplit (2 bits each).
11 11 11 11 => all subblocks are split both ways (3, 3, 3, 3), which together is 255 (11111111)
