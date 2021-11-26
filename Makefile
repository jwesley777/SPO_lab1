all:
	rm -rf fs_app
	gcc main.c fat32.c -o fs_app
