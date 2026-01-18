# Ultima Underworld Portrait Replacer
Tool for exporting and reimporting character portraits in to the Ultima Underworld games.

This should work for Ultima Underworld 1 and 2 but I've only tried it on the first one in full.

# How To Use

You will need 4 files from your Ultima Underworld DATA folder:

- BODIES.GR
- CHRBTNS.GR
- HEADS.GR
- PALS.DAT

Place them in the same directory as the executable then in a command prompt type

UltimaUWTool export

This will then extract the three files to bitmaps, you can then replace the portraits.
If you wish to maintain the colour palette the game uses, it will have also extracted a palette.bmp you can use as a reference.

It's important that you keep the dimensions the same and it is either a 24bit or 8bit uncompressed bmp.

Once you have replaced the files, you can run the following command:

UltimaUWTool import

This will then update the .GR files you placed in the executable folder, you will need to copy these back in to your Ultima Underworld folder.

Enjoy your game!

If you do find any bugs, feel free to report them, I may look in to it, I only did this for a bit of fun for some friends :)
