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