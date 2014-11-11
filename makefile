all:
	g++ -g `root-config --cflags --glibs` dice_game.cxx -o dice_game
